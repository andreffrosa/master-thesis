/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Author:
 * André Rosa (af.rosa@campus.fct.unl.pt
 * Under the guidance of:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2020
 *********************************************************/

#include "handlers.h"

#include "discovery_algorithm/discovery_algorithms.h"

#include <stdlib.h>
#include <stdio.h>
#include <uuid/uuid.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#include "debug.h"

#include "Yggdrasil.h"

#include "utility/seq.h"

static unsigned int compute_missed(unsigned int misses, unsigned long period_ms, struct timespec* exp_time, struct timespec* moment) {
    assert( compare_timespec(exp_time, moment) >= 0 );

    /*
    if( compare_timespec(exp_time, moment) < 0 ) {
        return misses;
    }
    */

    struct timespec start_time;
    milli_to_timespec(&start_time, period_ms*misses);
    subtract_timespec(&start_time, exp_time, &start_time);

    if( compare_timespec(moment, &start_time) < 0 ) {
        return 0;
    }

    struct timespec aux;
    subtract_timespec(&aux, exp_time, moment);
    unsigned long remaining = timespec_to_milli(&aux);

    unsigned int missed = (unsigned int)(misses - (((double)remaining) / period_ms));

    assert(missed <= misses);

    return missed;
}

static unsigned long compute_next_moment(struct timespec* exp_time, struct timespec* moment, unsigned int misses, unsigned int missed, unsigned long period) {
    assert( compare_timespec(exp_time, moment) >= 0 );

    struct timespec remaining;
    subtract_timespec(&remaining, exp_time, moment);

    unsigned long next_miss = timespec_to_milli(&remaining) - (misses - 1 - missed)*period;

    return next_miss;
}

void DF_dispatchMessage(queue_t* dispatcher_queue, YggMessage* msg) {
    queue_t_elem elem = {
			.data.msg = *msg,
			.type = YGG_MESSAGE
	};
    queue_push(dispatcher_queue, &elem);
}

void DF_init(discovery_framework_state* state) {

    getmyId(state->myID);
    memcpy(state->myAddr.data, getMyWLANAddr()->data, WLAN_ADDR_LEN);
    state->my_seq = 0;

    state->neighbors = newNeighborsTable();

    state->environment = newDiscoveryEnvironment(state->args->traffic_n_bucket, state->args->traffic_bucket_duration_s, state->args->churn_n_bucket, state->args->churn_bucket_duration_s);

    state->dirty_neighborhood = false;

    // Hello Timer
    genUUID(state->hello_timer_id);
    state->hello_timer_active = false;
    copy_timespec(&state->last_hello_time, &zero_timespec);
    copy_timespec(&state->next_hello_time, &zero_timespec);
    scheduleHelloTimer(state, true);

    // Hack Timer
    genUUID(state->hack_timer_id);
    state->hack_timer_active = false;
    copy_timespec(&state->last_hack_time, &zero_timespec);
    copy_timespec(&state->next_hack_time, &zero_timespec);
    scheduleHackTimer(state, true);

    // Neighbor Change Timer
    genUUID(state->neighbor_change_timer_id);
    state->neighbor_change_timer_active = false;
    //copy_timespec(&state->set_neighbor_change_time, &zero_timespec);
    //memset(&state->neighbor_change_summary, 0, sizeof(state->neighbor_change_summary));
    copy_timespec(&state->next_reactive_hello_time, &zero_timespec);
    copy_timespec(&state->next_reactive_hack_time, &zero_timespec);

    state->pending_notifications = list_init();

    // Stats
	memset(&state->stats, 0, sizeof(discovery_stats));

    // Discovery Environment Timer
    genUUID(state->discovery_environment_timer_id);
    if(state->args->discov_env_refresh_period_s > 0 && state->args->toggle_env) {
        struct timespec t_ = {0};
        milli_to_timespec(&t_, state->args->discov_env_refresh_period_s*1000);
        SetPeriodicTimer(&t_, state->discovery_environment_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, DISCOVERY_ENVIRONMENT_TIMER);
    }
}

void DF_createHello(discovery_framework_state* state, HelloMessage* hello, bool request_replies) {
    assert(hello);

    // Increment SEQ
    state->my_seq = inc_seq(state->my_seq, state->args->ignore_zero_seq);

    // Compute Next Hello Period
    struct timespec aux;
    subtract_timespec(&aux, &state->current_time, &state->last_hello_time);
    unsigned long elapsed_time_ms = timespec_to_milli(&aux);
    DA_computeNextHelloPeriod(state->args->algorithm, elapsed_time_ms, state->args->announce_transition_period_n, state->neighbors, &state->current_time);

    //DF_uponDiscoveryEnvironmentTimer(state); // Update and notify if necessary
    double out_traffic = DE_getOutTraffic(state->environment);

    // Create Hello
    initHelloMessage(hello,
        state->myID,
        state->my_seq,
        DA_getHelloAnnouncePeriod(state->args->algorithm),
        out_traffic,
        request_replies
    );

    /*
    #ifdef DEBUG_DISCOVERY
    char str[200];
    sprintf(str, "SEQ=%hu PERIOD=%ds", hello->seq, hello->period);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "CREATE HELLO", str);
    #endif
    */
}

void DF_createHack(discovery_framework_state* state, HackMessage* hack, NeighborEntry* neigh, bool single) {
    assert(neigh && hack);

    assert(!NE_isPending(neigh));

    if( single ) {
        // Compute Next Hack Period
        struct timespec aux;
        subtract_timespec(&aux, &state->current_time, &state->last_hack_time);
        unsigned long elapsed_time_ms = timespec_to_milli(&aux);
        DA_computeNextHackPeriod(state->args->algorithm, elapsed_time_ms, state->args->announce_transition_period_n, state->neighbors, &state->current_time);
    }

    // Create Standard Hack
    initHackMessage(hack, state->myID,
        NE_getNeighborID(neigh),
        NE_getNeighborSEQ(neigh),
        NE_getRxLinkQuality(neigh),
        NE_getTxLinkQuality(neigh),
        DA_getHackAnnouncePeriod(state->args->algorithm),
        NE_getOutTraffic(neigh),
        NE_getNeighborType(neigh, &state->current_time)
    );

    /*
    #ifdef DEBUG_DISCOVERY
    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse( , id_str);

    char str[200];
    sprintf(str, "ID=%s SEQ=%hu RX_LQ=%0.2f TX_LQ=%0.2f TYPE=%s PERIOD=%ds", id_str, hack_size);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "CREATE HACK", str);
    #endif
    */
}

void DF_createHackBatch(discovery_framework_state* state, HackMessage** hacks, byte* n_hacks, NeighborsTable* neighbors) {


    if( NT_getSize(state->neighbors) > 0 ) {
        *hacks = malloc(NT_getSize(state->neighbors) * sizeof(HackMessage));

        // Compute Next Hack Period
        struct timespec aux;
        subtract_timespec(&aux, &state->current_time, &state->last_hack_time);
        unsigned long elapsed_time_ms = timespec_to_milli(&aux);
        DA_computeNextHackPeriod(state->args->algorithm, elapsed_time_ms, state->args->announce_transition_period_n, state->neighbors, &state->current_time);

        byte counter = 0;

        void* iterator = NULL;
        NeighborEntry* current_neigh = NULL;
        HackMessage* current_hack = *hacks;
        while ( (current_neigh = NT_nextNeighbor(state->neighbors, &iterator)) ) {
            if( !NE_isPending(current_neigh) ) {
                DF_createHack(state, current_hack++, current_neigh, false);
                counter++;
            }
        }

        if(counter == 0) {
            free(*hacks);
            *hacks = NULL;
        }

        *n_hacks = counter;
    } else {
        *n_hacks = 0;
        *hacks = NULL;
    }

}

void DF_processMessage(discovery_framework_state* state, byte* data, unsigned short size, bool piggybacked, WLANAddr* mac_addr) {

    MessageSummary* msg_summary = newMessageSummary();

    bool neighbor_change = DA_processMessage(state->args->algorithm, state, state->myID, state->neighbors, &state->current_time, piggybacked, mac_addr, data, size, msg_summary);

    assert(msg_summary->hello_summary || msg_summary->hack_summaries->size > 0);

    NeighborEntry* neigh = NULL;
    if(msg_summary->hello_summary) {
        neigh = msg_summary->hello_summary->neigh;
    } else {
        neigh = ((HackDeliverSummary*)msg_summary->hack_summaries->head->data)->neigh;
    }
    //assert(neigh != NULL);

    if(neigh) {

        for(list_item* it = msg_summary->hack_summaries->head; it; it = it->next) {
            HackDeliverSummary* hack_summary = (HackDeliverSummary*)it->data;

            if(hack_summary->neigh) {
                assert(neigh == hack_summary->neigh);
            }
        }

        if(NE_isPending(neigh)) {
            DF_printNeighbors(state);
        }

        // Re-schedule neighbor timer
        scheduleNeighborTimer(state, neigh);

        //bool new_neighbor = msg_summary->hello_summary && msg_summary->hello_summary->new_neighbor;
        if(msg_summary->hello_summary && msg_summary->hello_summary->became_accepted /*new_neighbor*/ ) {
            DF_notifyNewNeighbor(state, neigh);

            state->stats.new_neighbors++;

            DE_registerNewNeighbor(state->environment, &state->current_time);

            scheduleNeighborChange(state, msg_summary->hello_summary, NULL, NULL, false);
        } else {
            bool updated_neighbor = msg_summary->hello_summary && msg_summary->hello_summary->updated_neighbor;
            if( updated_neighbor ) {
                if( msg_summary->hello_summary->updated_quality_threshold || msg_summary->hello_summary->updated_traffic_threshold || msg_summary->hello_summary->rebooted ) {
                    scheduleNeighborChange(state, msg_summary->hello_summary, NULL, NULL, false);
                }
            }

            for(list_item* it = msg_summary->hack_summaries->head; it; it = it->next) {
                HackDeliverSummary* hack_summary = (HackDeliverSummary*)it->data;

                if(hack_summary->neigh) {
                    assert(neigh == hack_summary->neigh);


                    bool aux = (hack_summary->updated_neighbor || hack_summary->updated_2hop_neighbor || hack_summary->added_2hop_neighbor || hack_summary->lost_2hop_neighbor);

                    updated_neighbor |= aux;

                    if(aux) {
                        if( hack_summary->became_bi || hack_summary->lost_bi || hack_summary->updated_quality_threshold || (hack_summary->became_bi_2hop || hack_summary->lost_bi_2hop || hack_summary->updated_2hop_quality_threshold || hack_summary->updated_2hop_traffic_threshold) || hack_summary->added_2hop_neighbor || hack_summary->lost_2hop_neighbor ) {
                            scheduleNeighborChange(state, NULL, hack_summary, NULL, false);
                        }
                    }
                }
            }

            if( updated_neighbor ) {
                if( !NE_isPending(neigh) ) {
                    DF_notifyUpdateNeighbor(state, neigh);
                } else {
                    DF_printNeighbors(state);
                }
            }
        }

        if(neighbor_change) {
            scheduleNeighborChange(state, false, false, false, true);
        }
    }

    if(msg_summary->hello_summary) free(msg_summary->hello_summary);
    list_delete(msg_summary->hack_summaries);
    free(msg_summary);
}

void scheduleHelloTimer(discovery_framework_state* state, bool now) {

    if( DA_periodicHello(state->args->algorithm) != NO_PERIODIC ) {

        // Compute jitter
        unsigned long hello_period_ms = DA_getHelloTransmitPeriod(state->args->algorithm, &state->current_time)*1000;

        unsigned long real_period_ms = hello_period_ms - state->args->period_margin_ms;

        struct timespec max_next_hello_time = {0};
        milli_to_timespec(&max_next_hello_time, real_period_ms);
        add_timespec(&max_next_hello_time, &max_next_hello_time, &state->last_hello_time);

        struct timespec allowed_max_jitter_ = {0};
        subtract_timespec(&allowed_max_jitter_, &max_next_hello_time, &state->current_time);

        unsigned long allowed_max_jitter_ms = timespec_to_milli(&allowed_max_jitter_);

        unsigned long max_jitter_ms = ulMin(state->args->max_jitter_ms, allowed_max_jitter_ms);

        unsigned long jitter = (unsigned long)(randomProb()*max_jitter_ms);

        unsigned long t = now ? jitter : real_period_ms - jitter;

        struct timespec t_ = {0};
        milli_to_timespec(&t_, t);

        if( DA_periodicHello(state->args->algorithm) == STATIC_PERIODIC ) {
            if( !state->hello_timer_active ) {
                state->hello_timer_active = true;

                SetTimer(&t_, state->hello_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HELLO_TIMER);

                add_timespec(&state->next_hello_time, &t_, &state->current_time);
            } else {
                // Ignore
            }
        } else if( DA_periodicHello(state->args->algorithm) == RESET_PERIODIC ) {
            if( !state->hello_timer_active ) {
                state->hello_timer_active = true;

                SetTimer(&t_, state->hello_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HELLO_TIMER);
            }

            add_timespec(&state->next_hello_time, &t_, &state->current_time);
        }
    }
}

void scheduleHackTimer(discovery_framework_state* state, bool now) {

    if( DA_periodicHack(state->args->algorithm) != NO_PERIODIC ) {

        // Compute jitter
        unsigned long hack_period_ms = DA_getHackTransmitPeriod(state->args->algorithm, &state->current_time)*1000;

        unsigned long real_period_ms = hack_period_ms - state->args->period_margin_ms;

        struct timespec max_next_hack_time = {0};
        milli_to_timespec(&max_next_hack_time, real_period_ms);
        add_timespec(&max_next_hack_time, &max_next_hack_time, &state->last_hack_time);

        struct timespec allowed_max_jitter_ = {0};
        subtract_timespec(&allowed_max_jitter_, &max_next_hack_time, &state->current_time);

        unsigned long allowed_max_jitter_ms = timespec_to_milli(&allowed_max_jitter_);

        unsigned long max_jitter_ms = ulMin(state->args->max_jitter_ms, allowed_max_jitter_ms);

        unsigned long jitter = (unsigned long)(randomProb()*max_jitter_ms);

        unsigned long t = now ? jitter : real_period_ms - jitter;

        struct timespec t_ = {0};
        milli_to_timespec(&t_, t);

        if( DA_periodicHack(state->args->algorithm) == STATIC_PERIODIC ) {
            if( !state->hack_timer_active ) {
                state->hack_timer_active = true;

                SetTimer(&t_, state->hack_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HACK_TIMER);

                add_timespec(&state->next_hack_time, &t_, &state->current_time);
            } else {
                // Ignore
            }
        } else if( DA_periodicHack(state->args->algorithm) == RESET_PERIODIC ) {
            if( !state->hack_timer_active ) {
                state->hack_timer_active = true;

                SetTimer(&t_, state->hack_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HACK_TIMER);
            }

            add_timespec(&state->next_hack_time, &t_, &state->current_time);
        }
    }
}

void DF_uponHelloTimer(discovery_framework_state* state) {
    assert( state->hello_timer_active );

    if( DA_periodicHello(state->args->algorithm) ) {

        if( compare_timespec(&state->current_time, &state->next_hello_time) >= 0 ) {
            state->hello_timer_active = false;

            YggMessage msg = {0};
            DiscoverySendPack* dsp = DF_triggerDiscoveryEvent(state, DPE_HELLO_TIMER, NULL, &msg);
            assert(dsp);

            #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
                char hack_str[10];
                if( dsp->hacks ) {
                    sprintf(hack_str, "%d", dsp->n_hacks);
                } else {
                    sprintf(hack_str, "-");
                }

                char str[200];
                sprintf(str, "HELLO=[SEQ=%hu] HACK=[%s]", dsp->hello->seq, hack_str);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "HELLO TIMER", str);
            #endif

            if(dsp->hacks) {
                state->stats.piggybacked_hacks += dsp->n_hacks;
            }

            DF_sendMessage(state, dsp, &msg);

            // Re-schedule
            // scheduleHelloTimer(state, false);

        } else {
            struct timespec remaining;
            subtract_timespec(&remaining, &state->next_hello_time, &state->current_time);

            SetTimer(&remaining, state->hello_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HELLO_TIMER);
        }

    } else {
        // TODO: ignore the timer or display error?
        assert(false);
    }
}

void DF_uponHackTimer(discovery_framework_state* state) {
    assert( state->hack_timer_active );

    if( DA_periodicHack(state->args->algorithm) ) {

        if( compare_timespec(&state->current_time, &state->next_hack_time) >= 0 ) {
            {
                unsigned long hack_period_ms = DA_getHackTransmitPeriod(state->args->algorithm, &state->current_time)*1000;
                unsigned long real_period_ms = hack_period_ms - state->args->period_margin_ms;

                struct timespec diff_ = {0};
                subtract_timespec(&diff_, &state->current_time, &state->last_hack_time);
                unsigned long diff = timespec_to_milli(&diff_);

                bool first_timer = compare_timespec(&state->last_hack_time, (struct timespec*)&zero_timespec) == 0;

                assert( first_timer ? true : diff <= real_period_ms );
            }

            state->hack_timer_active = false;
            // copy_timespec(&state->last_hack_time, &state->current_time);

            YggMessage msg = {0};
            DiscoverySendPack* dsp = DF_triggerDiscoveryEvent(state, DPE_HACK_TIMER, NULL, &msg);

            #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
                char hello_str[10];
                if( dsp && dsp->hello ) {
                    sprintf(hello_str, "SEQ=%hu", dsp->hello->seq);
                } else {
                    sprintf(hello_str, "-");
                }

                char hack_str[10];
                if( dsp && dsp->n_hacks > 0 ) {
                    sprintf(hack_str, "%d", dsp->n_hacks);
                } else {
                    sprintf(hack_str, "-");
                }

                char str[200];
                sprintf(str, "HELLO=[%s] HACK=[%s]", hello_str, hack_str);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "HACK TIMER", str);
            #endif

            if(dsp) {
                if(dsp->hello) {
                    state->stats.piggybacked_hellos++;
                }

                if(dsp->n_hacks == 0) {
                    scheduleHackTimer(state, false);
                }
            }

            DF_sendMessage(state, dsp, &msg);

            // Re-schedule
            //scheduleHackTimer(state, false);
        } else {
            struct timespec remaining;
            subtract_timespec(&remaining, &state->next_hack_time, &state->current_time);

            SetTimer(&remaining, state->hack_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HACK_TIMER);
        }

    } else {
        // TODO: ignore the timer or display error?
        assert(false);
    }
}

void DF_uponReplyTimer(discovery_framework_state* state, unsigned char* timer_payload, unsigned short timer_payload_size) {
    assert(timer_payload);

    YggMessage msg = {0};
    DiscoverySendPack* dsp = DF_triggerDiscoveryEvent(state, DPE_REPLY_TIMER, timer_payload, &msg);
    assert(dsp);

    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
        char hello_str[10];
        if( dsp->hello ) {
            sprintf(hello_str, "SEQ=%hu", dsp->hello->seq);
        } else {
            sprintf(hello_str, "-");
        }

        char hack_str[10];
        if( dsp->n_hacks > 0 ) {
            sprintf(hack_str, "%d", dsp->n_hacks);
        } else {
            sprintf(hack_str, "-");
        }

        char str[200];
        sprintf(str, "HELLO=[%s] HACK=[%s]", hello_str, hack_str);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REPLY TIMER", str);
    #endif

    if(dsp->hello) {
        state->stats.piggybacked_hellos++;
    }

    DF_sendMessage(state, dsp, &msg);
}

bool DF_uponNeighborTimer(discovery_framework_state* state, unsigned char* neigh_id) {
    NeighborEntry* neigh = NT_getNeighbor(state->neighbors, neigh_id);
    //assert(neigh);

    if(neigh == NULL) {
        #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(neigh_id, id_str);

        char str[200];
        sprintf(str, "Neigh %s was deleted before! (ERROR)", id_str);

        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBOR TIMER", str);
        #endif

        return true;
    }

    NeighborTimerSummary* summary = newNeighborTimerSummary();
    summary->neigh = neigh;

    // Last Timer
    struct timespec last_neighbor_timer;
    copy_timespec(&last_neighbor_timer, NE_getLastNeighborTimer(neigh));
    NE_setLastNeighborTimer(neigh, &state->current_time);

    unsigned long next_timer_ms = ULONG_MAX;

    // If neighbor is Lost
    if( NE_isLost(neigh) ) {
        // Check if
        struct timespec* removal_time = NE_getNeighborRemovalTime(neigh);
        if( compare_timespec(removal_time, &state->current_time) <= 0 ) {
            #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
            char id_str[UUID_STR_LEN+1];
            id_str[UUID_STR_LEN] = '\0';
            uuid_unparse(NE_getNeighborID(neigh), id_str);

            char str[200];
            sprintf(str, "%s    (REMOVAL TIME)", id_str);

            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REMOVED NEIGHBOR", str);
            #endif

            flushNeighbor(state, neigh);
            neigh = NULL;

            summary->removed = true;
            summary->neigh = NULL;
        } else {
            struct timespec aux = {0};
            subtract_timespec(&aux, removal_time, &state->current_time);
            next_timer_ms = ulMin(next_timer_ms, timespec_to_milli(&aux));
        }
    }

    // If neigh was not removed
    if( !summary->removed ) {

        unsigned int hello_misses = state->args->hello_misses;
        unsigned int hack_misses = state->args->hack_misses;

        unsigned int missed_hellos = 0;
        unsigned int missed_hacks = 0;

        // Check if rx expired
        struct timespec* rx_exp_time = NE_getNeighborRxExpTime(neigh);
        if( compare_timespec(rx_exp_time, &state->current_time) <= 0 ) {

            if( !NE_isLost(neigh) ) {
                NE_setLost(neigh, &state->current_time);

                state->stats.lost_neighbors++;

                summary->lost_neighbor = true;

                struct timespec removal_time;
                milli_to_timespec(&removal_time, state->args->neigh_hold_time_s*1000);
                add_timespec(&removal_time, &removal_time,  &state->current_time);
                NE_setNeighborRemovalTime(neigh, &removal_time);

                next_timer_ms = ulMin(next_timer_ms, state->args->neigh_hold_time_s*1000);
            }

        } else { // Check if missed an hello
            // Compute missed hellos
            unsigned long hello_period = NE_getNeighborHelloPeriod(neigh)*1000;

            missed_hellos = compute_missed(hello_misses, hello_period, rx_exp_time, &state->current_time);

            unsigned int prev_missed_hellos = compute_missed(hello_misses, hello_period, rx_exp_time, &last_neighbor_timer);

            if( missed_hellos > prev_missed_hellos ) {
                unsigned int lost = 1;
                summary->missed_hellos++;

                state->stats.missed_hellos++;

                // Update Link Quality
                double old_rx_lq = NE_getRxLinkQuality(neigh);
                double new_rx_lq = DA_computeLinkQuality(state->args->algorithm, NE_getLinkQualityAttributes(neigh), old_rx_lq, 0, lost, false, &state->current_time);
                new_rx_lq = roundPrecision(new_rx_lq, state->args->lq_precision);

                double lq_delta = fabs(old_rx_lq - new_rx_lq);
                if( lq_delta >= state->args->lq_epsilon || (lq_delta > 0 && (new_rx_lq == 1.0 || new_rx_lq == 0.0)) ) {
                    NE_setRxLinkQuality(neigh, new_rx_lq);
                    summary->updated_quality = true;

                    if( lq_delta >= state->args->lq_threshold ) {
                        summary->updated_quality_threshold = true;
                    }
                }

                // Update Link Admission
                bool old_accepted = NE_isAccepted(neigh);
                bool accepted = DA_evalLinkAdmission(state->args->algorithm, neigh, &state->current_time);
                NE_setAccepted(neigh, accepted);

                //bool became_accepted = !old_accepted && accepted;
                bool lost_accepted = old_accepted && !accepted;

                //assert(!became_accepted);

                if(lost_accepted) {
                    assert(!NE_isPending(neigh));

                    // Neighbor is dead
                    if( !NE_isLost(neigh) ) {
                        NE_setLost(neigh, &state->current_time);

                        state->stats.lost_neighbors++;

                        summary->lost_neighbor = true;

                        struct timespec removal_time;
                        milli_to_timespec(&removal_time, state->args->neigh_hold_time_s*1000);
                        add_timespec(&removal_time, &removal_time,  &state->current_time);
                        NE_setNeighborRemovalTime(neigh, &removal_time);

                        next_timer_ms = ulMin(next_timer_ms, state->args->neigh_hold_time_s*1000);
                    } else {
                        assert(false); // Error
                    }
                }
            }

            unsigned long next_hello_miss = compute_next_moment(rx_exp_time, &state->current_time, hello_misses, missed_hellos, hello_period);
            next_timer_ms = ulMin(next_timer_ms, next_hello_miss);
        }

        // Check if tx not expired
        struct timespec* tx_exp_time = NE_getNeighborTxExpTime(neigh);
        if( compare_timespec(tx_exp_time, &state->current_time) <= 0 ) {
            // Check if tx expired after the previous neighbor timer
            if( compare_timespec(tx_exp_time, &last_neighbor_timer) > 0 ) {
                summary->lost_bi = true;

                // Log
                #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(NE_getNeighborID(neigh), id_str);

                char str[200];
                sprintf(str, "%s", id_str);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "LOST BI", str);
                #endif
            }

        } else {
            unsigned long hack_period = NE_getNeighborHackPeriod(neigh)*1000;

            missed_hacks = compute_missed(hack_misses, hack_period, tx_exp_time, &state->current_time);

            unsigned int prev_missed_hacks = compute_missed(hack_misses, hack_period, tx_exp_time, &last_neighbor_timer);

            if( missed_hacks > prev_missed_hacks ) {
                state->stats.missed_hacks++;

                summary->missed_hacks++;
            }

            unsigned long next_hack_miss = compute_next_moment(tx_exp_time, &state->current_time, hack_misses, missed_hacks, hack_period);
            next_timer_ms = ulMin(next_timer_ms, next_hack_miss);
        }

        struct timespec removal_time = {0};
        milli_to_timespec(&removal_time, state->args->neigh_hold_time_s*1000);
        add_timespec(&removal_time, &removal_time,  &state->current_time);

        // 2-hop Neighs GC + compute next expiration
        hash_table* two_hop_neighbors = NE_getTwoHopNeighbors(neigh);
        void* iterator = NULL;
        hash_table_item* hit = NULL;
        while( (hit = hash_table_iterator_next(two_hop_neighbors, &iterator)) ) {
            TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;

            struct timespec* two_hop_neigh_exp_time = THNE_getExpiration(nn);
            if( compare_timespec(two_hop_neigh_exp_time, &state->current_time) < 0 ) {
                if( THNE_isLost(nn) ) {
                    hash_table_remove_item(two_hop_neighbors, THNE_getID(nn));
                    free(nn);
                    free(hit);
                } else {
                    THNE_setLost(nn, true);
                    THNE_setExpiration(nn, &removal_time);
                    summary->deleted_2hop++;

                    next_timer_ms = ulMin(next_timer_ms, timespec_to_milli(&removal_time));
                }
            } else {
                struct timespec aux;
                subtract_timespec(&aux, two_hop_neigh_exp_time, &state->current_time);

                next_timer_ms = ulMin(next_timer_ms, timespec_to_milli(&aux));
            }
        }
        summary->lost_2hop_neighbor = summary->deleted_2hop > 0;

        summary->updated_neighbor = summary->lost_bi || summary->updated_quality;

        // Log
        #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
        if(summary->missed_hellos > 0 || summary->missed_hacks > 0 || summary->deleted_2hop > 0) {
            char id_str[UUID_STR_LEN+1];
            id_str[UUID_STR_LEN] = '\0';
            uuid_unparse(NE_getNeighborID(neigh), id_str);

            char hellos_str[20];
            if( compare_timespec(rx_exp_time, &state->current_time) > 0 ) {
                if(summary->missed_hellos > 0) {
                    sprintf(hellos_str, "%u/%u(+1)", missed_hellos, hello_misses);
                } else {
                    sprintf(hellos_str, "%u/%u", missed_hellos, hello_misses);
                }
            } else {
                sprintf(hellos_str, "expired%s", (summary->missed_hellos > 0?"(+1)":""));
            }

            char hacks_str[20];
            if(compare_timespec(tx_exp_time, &state->current_time) > 0) {
                if(summary->missed_hacks > 0) {
                    sprintf(hacks_str, "%u/%u(+1)", missed_hacks, hack_misses);
                } else {
                    sprintf(hacks_str, "%u/%u", missed_hacks, hack_misses);
                }
            } else {
                //if() {
                //    sprintf(hacks_str, "lost bi");
                //} else {
                //    sprintf(hacks_str, "expired");
                //}
                sprintf(hacks_str, "expired%s", (summary->missed_hacks > 0?"(+1)":""));
            }

            char str[200];
            sprintf(str, "%s  missed hellos: %s  missed hacks: %s  lost 2-hop neighs: %u", id_str, hellos_str, hacks_str, summary->deleted_2hop);
            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBOR TIMER", str);
        }
        #endif

        bool changed = false;
        bool schedule = false;

        if( summary->lost_neighbor ) {
            if(!NE_isPending(neigh)) {
                DF_notifyLostNeighbor(state, neigh);

                DE_registerLostNeighbor(state->environment, &state->current_time);
            }

            changed = true;
            schedule = true;

        } else {
            if( summary->updated_neighbor || summary->lost_2hop_neighbor ) {
                if(!NE_isPending(neigh)) {
                    DF_notifyUpdateNeighbor(state, neigh);
                } else {
                    DF_printNeighbors(state);
                }

                changed = true;

                if( summary->lost_bi || summary->updated_quality_threshold || summary->lost_2hop_neighbor ) {
                    schedule = true;
                }
            }

        }

        if( changed ) {
            bool updated_context = DA_updateContext(state->args->algorithm, state, state->myID, neigh, state->neighbors, &state->current_time, summary);

            schedule |= (updated_context && ( DA_HelloContextUpdate(state->args->algorithm) || DA_HackContextUpdate(state->args->algorithm) ));

            if( schedule ) {
                scheduleNeighborChange(state, NULL, NULL, summary, updated_context);
            }
        }

        //scheduleNeighborTimer(state, neigh);
        struct timespec t;
        milli_to_timespec(&t, next_timer_ms);
        SetTimer(&t, NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_TIMER);
    }

    bool removed = summary->removed;

    free(summary);

    return removed;
}

void DF_uponDiscoveryEnvironmentTimer(discovery_framework_state* state) {
    //assert(state->args->toggle_env);

    bool changed = false;

    unsigned int n_neighbors = 0;
    double neighbors_density = 0.0;
    double in_traffic = 0.0;
    void* iterator = NULL;
    for(NeighborEntry* current_neigh = NT_nextNeighbor(state->neighbors, &iterator); current_neigh; current_neigh = NT_nextNeighbor(state->neighbors, &iterator)) {
        DiscoveryNeighborType neigh_type = NE_getNeighborType(current_neigh, &state->current_time);
        if( neigh_type != LOST_NEIGH && neigh_type != PENDING_NEIGH ) {
            in_traffic += NE_getOutTraffic(current_neigh);
            n_neighbors++;
            neighbors_density += NE_getTwoHopNeighbors(current_neigh)->n_items;
        }
    }

    if( n_neighbors > 0 ) {
        neighbors_density += n_neighbors;
        neighbors_density /= (n_neighbors+1);
    }

    neighbors_density = roundPrecision(neighbors_density, state->args->neigh_density_precision);

    changed |= DE_setInTraffic(state->environment, in_traffic, state->args->traffic_epsilon);

    changed |= DE_computeOutTraffic(state->environment, &state->current_time, state->args->traffic_window_type, state->args->traffic_epsilon, state->args->traffic_precision);

    changed |= DE_computeNewNeighborsFlux(state->environment, &state->current_time, state->args->churn_window_type, state->args->churn_epsilon, state->args->churn_precision);

    changed |= DE_computeLostNeighborsFlux(state->environment, &state->current_time, state->args->churn_window_type, state->args->churn_epsilon, state->args->churn_precision);

    changed |= DE_setNNeighbors(state->environment, n_neighbors);

    changed |= DE_setNeigbhorsDensity(state->environment, neighbors_density, state->args->neigh_density_epsilon);

    if( changed && state->args->toggle_env ) {
        // Log
        #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
        char* str = NULL;

        NE_print(state->environment, &str);

        char str2[strlen(str)+1];
        sprintf(str2, "\n%s", str);

        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "DISCOVERY ENVIRONMENT", str2);

        free(str);
        #endif

        DF_notifyDiscoveryEnvironment(state);
    }
}

void scheduleNeighborTimer(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    unsigned long next_timer_ms = ULONG_MAX;

    // Check if rx not expired
    struct timespec* rx_exp_time = NE_getNeighborRxExpTime(neigh);
    if( compare_timespec(rx_exp_time, &state->current_time) > 0 ) {
        // Compute missed hellos
        unsigned long hello_period = NE_getNeighborHelloPeriod(neigh)*1000;
        unsigned int hello_misses = state->args->hello_misses;

        unsigned int missed_hellos = compute_missed(hello_misses, hello_period, rx_exp_time, &state->current_time);

        unsigned long next_hello_miss = compute_next_moment(rx_exp_time, &state->current_time, hello_misses, missed_hellos, hello_period);

        next_timer_ms = ulMin(next_timer_ms, next_hello_miss);
    }

    // Check if tx not expired
    struct timespec* tx_exp_time = NE_getNeighborTxExpTime(neigh);
    if( compare_timespec(tx_exp_time, &state->current_time) > 0 ) {
        // Compute missed hacks
        unsigned long hack_period = NE_getNeighborHackPeriod(neigh)*1000;
        unsigned int hack_misses = state->args->hack_misses;

        unsigned int missed_hacks = compute_missed(hack_misses, hack_period, tx_exp_time, &state->current_time);

        unsigned long next_hack_miss = compute_next_moment(tx_exp_time, &state->current_time, hack_misses, missed_hacks, hack_period);

        next_timer_ms = ulMin(next_timer_ms, next_hack_miss);
    }

    // Check if removal time
    if( NE_isLost(neigh) ) {
        struct timespec* removal_time = NE_getNeighborTxExpTime(neigh);

        // If already expired
        if( compare_timespec(removal_time, &state->current_time) <= 0 ) {
            next_timer_ms = 0;
        } else {
            struct timespec aux;
            subtract_timespec(&aux, removal_time, &state->current_time);

            next_timer_ms = ulMin(next_timer_ms, timespec_to_milli(&aux));
        }
    }

    // Check 2-hop Neighbors
    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(NE_getTwoHopNeighbors(neigh), &iterator)) ) {
        TwoHopNeighborEntry* two_hop_neigh = (TwoHopNeighborEntry*)hit->value;

        struct timespec* two_hop_neigh_exp_time = THNE_getExpiration(two_hop_neigh);

        // If already expired
        if( compare_timespec(two_hop_neigh_exp_time, &state->current_time) <= 0 ) {
            next_timer_ms = 0;
        } else {
            struct timespec aux;
            subtract_timespec(&aux, two_hop_neigh_exp_time, &state->current_time);

            next_timer_ms = ulMin(next_timer_ms, timespec_to_milli(&aux));
        }
    }

    if( next_timer_ms > 0 ) {
        // TODO: compare with old timer
        CancelTimer(NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID);
        struct timespec t;
        milli_to_timespec(&t, next_timer_ms);
        SetTimer(&t, NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_TIMER);


        //char id_str[UUID_STR_LEN];
        //uuid_unparse(NE_getNeighborID(neigh), id_str);
        //printf("Re-scheduling neighbor timer of %s (next_timer = %lu ms > 0)\n", id_str, next_timer_ms);
    }

}

void flushNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    NT_removeNeighbor(state->neighbors, NE_getNeighborID(neigh));

    CancelTimer(NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID);

    void* lq_attrs = NULL, *msg_attrs = NULL;
    destroyNeighborEntry(neigh, &lq_attrs, &msg_attrs);

    DA_destroyLinkQualityAttributes(state->args->algorithm, lq_attrs);
    DA_destroyContextAttributes(state->args->algorithm, msg_attrs);
}

void scheduleReply(discovery_framework_state* state, HelloMessage* hello, HelloDeliverSummary* summary) {

    if( DA_replyHacksToHellos(state->args->algorithm) != NO_HACK_REPLY  && hello->request_replies ) {

        unsigned long jitter = (unsigned long)(randomProb()*state->args->max_jitter_ms);

        struct timespec t_ = {0};
        milli_to_timespec(&t_, jitter);

        unsigned int buffer_size = sizeof(uuid_t) + sizeof(bool);
        byte buffer[buffer_size];
        memcpy(buffer, hello->process_id, sizeof(uuid_t));
        memcpy(buffer+sizeof(uuid_t), &summary->new_neighbor, sizeof(bool));

        SetTimerWithPayload(&t_, NULL, DISCOVERY_FRAMEWORK_PROTO_ID, REPLY_TIMER, buffer, buffer_size);
    }

}

void DF_uponNeighborChangeTimer(discovery_framework_state* state) {
    assert( state->neighbor_change_timer_active );

    bool send_hello = compare_timespec(&state->next_reactive_hello_time, (struct timespec*)&zero_timespec) != 0 && compare_timespec(&state->next_reactive_hello_time, &state->current_time) <= 0;

    bool send_hack = compare_timespec(&state->next_reactive_hack_time, (struct timespec*)&zero_timespec) != 0 && compare_timespec(&state->next_reactive_hack_time, &state->current_time) <= 0;

    assert(send_hello || send_hack);

    bool aux[2] = {send_hello, send_hack};

    YggMessage msg = {0};
    DiscoverySendPack* dsp = DF_triggerDiscoveryEvent(state, DPE_NEIGHBORHOOD_CHANGE_TIMER, aux, &msg);
    assert(dsp);

    // Log
    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
    char hello_str[10];
    if( dsp->hello ) {
        sprintf(hello_str, "SEQ=%hu", dsp->hello->seq);
    } else {
        sprintf(hello_str, "-");
    }

    char hack_str[10];
    if( dsp->n_hacks > 0 ) {
        sprintf(hack_str, "%d", dsp->n_hacks);
    } else {
        sprintf(hack_str, "-");
    }

    char str[200];
    sprintf(str, "HELLO=[%s] HACK=[%s]", hello_str, hack_str);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBOR-CHANGE TIMER", str);
    #endif

    if(send_hello) {
        copy_timespec(&state->next_reactive_hello_time, &zero_timespec);
    }

    if(send_hack) {
        copy_timespec(&state->next_reactive_hack_time, &zero_timespec);
    }

    bool hello_is_set = compare_timespec(&state->next_reactive_hello_time, (struct timespec*)&zero_timespec) != 0;
    bool hack_is_set = compare_timespec(&state->next_reactive_hack_time, (struct timespec*)&zero_timespec) != 0;

    struct timespec next_timer = {0};
    if(hello_is_set && !hack_is_set) {
        copy_timespec(&next_timer, &state->next_reactive_hello_time);
    } else if(!hello_is_set && hack_is_set) {
        copy_timespec(&next_timer, &state->next_reactive_hack_time);
    } else if(hello_is_set || hack_is_set) {
        if( compare_timespec(&state->next_reactive_hello_time, &state->next_reactive_hack_time) < 0 ) {
            copy_timespec(&next_timer, &state->next_reactive_hello_time);
        } else {
            copy_timespec(&next_timer, &state->next_reactive_hack_time);
        }
    } /*else {
        assert(!state->neighbor_change_timer_active);
    }*/

    if(hello_is_set || hack_is_set) {
        struct timespec t = {0};
        subtract_timespec(&t, &next_timer, &state->current_time);
        SetTimer(&t, state->neighbor_change_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_CHANGE_TIMER);
    } else {
        state->neighbor_change_timer_active = false;
    }

    DF_sendMessage(state, dsp, &msg);
}

bool computeNextTimer(unsigned long max_jitter_ms, unsigned long min_interval_ms, struct timespec* current_time, struct timespec* last_timer, struct timespec* next_timer, struct timespec* other_timer, struct timespec* t) {
    struct timespec min_interval = {0};
    milli_to_timespec(&min_interval, min_interval_ms);

    struct timespec t_max = {0};
    subtract_timespec(&t_max, next_timer, &min_interval);

    struct timespec t_min = {0};
    add_timespec(&t_min, last_timer, &min_interval);

    if( compare_timespec(&t_min, &t_max) < 0 && compare_timespec(current_time, &t_max) < 0 ) {
        struct timespec diff_ = {0};
        subtract_timespec(&diff_, &t_max, &t_min);
        unsigned long diff = timespec_to_milli(&diff_);

        struct timespec diff2_ = {0};
        subtract_timespec(&diff2_, &t_max, current_time);
        unsigned long diff2 = timespec_to_milli(&diff2_);

        bool set_timer = diff >= max_jitter_ms && diff2 >= max_jitter_ms;

        if(set_timer) {
            struct timespec jitter = {0};
            milli_to_timespec(&jitter, (unsigned long)(randomProb()*max_jitter_ms));

            if( compare_timespec(current_time, &t_min) < 0 ) {
                struct timespec remaining = {0};
                subtract_timespec(&remaining, &t_min, current_time);
                add_timespec(t, &remaining, &jitter);
                return true;
            } else {
                struct timespec close = {0};
                milli_to_timespec(&close, max_jitter_ms);
                add_timespec(&close, current_time, &close);

                if( other_timer && compare_timespec(other_timer, &close) <= 0 ) {
                    assert(compare_timespec(other_timer, current_time) >= 0);
                    subtract_timespec(t, other_timer, current_time);
                    return true;
                } else {
                    copy_timespec(t, &jitter);
                    return true;
                }
            }
        }
    }

    return false;
}

void scheduleNeighborChange(discovery_framework_state* state, HelloDeliverSummary* hello_summary, HackDeliverSummary* hack_summary, NeighborTimerSummary* neighbor_timer_summary, bool context_updates) {

    NeighborChangeSummary summary = {0};

    if( hello_summary ) {
        summary.new_neighbor = hello_summary->new_neighbor;
        summary.updated_neighbor = hello_summary->updated_neighbor;
        summary.lost_neighbor = hello_summary->lost_neighbor;
        summary.rebooted = hello_summary->rebooted;
        summary.hello_period_changed = hello_summary->period_changed;
        summary.updated_quality = hello_summary->updated_quality;
        summary.updated_quality_threshold = hello_summary->updated_quality_threshold;
        // summary->updated_traffic |= hello_summary->updated_traffic;
        // summary->updated_traffic_threshold |= hello_summary->updated_traffic_threshold;
        // summary->missed_hellos += hello_summary->misseds_hellos;
    }

    if( hack_summary ) {
        summary.updated_neighbor = hack_summary->updated_neighbor;
        // summary->missed_hacks += hack_summary->missed_hacks;
        summary.became_bi = hack_summary->became_bi;
        summary.lost_bi = hack_summary->lost_bi;
        summary.hack_period_changed = hack_summary->period_changed;
        summary.updated_quality = hack_summary->updated_quality;
        summary.updated_quality_threshold = hack_summary->updated_quality_threshold;
        summary.updated_2hop_neighbor = hack_summary->updated_2hop_neighbor;
        summary.added_2hop_neighbor = hack_summary->added_2hop_neighbor;
        summary.lost_2hop_neighbor = hack_summary->lost_2hop_neighbor;
    }

    if( neighbor_timer_summary ) {
        summary.updated_neighbor = neighbor_timer_summary->updated_neighbor;
        summary.lost_neighbor = neighbor_timer_summary->lost_neighbor;
        summary.removed = neighbor_timer_summary->removed;
        summary.lost_bi = neighbor_timer_summary->lost_bi;
        summary.updated_quality = neighbor_timer_summary->updated_quality;
        summary.updated_quality_threshold = neighbor_timer_summary->updated_quality_threshold;
        //summary->deleted_2hop += neighbor_timer_summary->deleted_2hop;
        //summary->missed_hellos  += neighbor_timer_summary->missed_hellos;
        //summary->missed_hacks  += neighbor_timer_summary->missed_hacks;

        //summary->updated_2hop_neighbor |= neighbor_timer_summary->updated_2hop_neighbor;
        summary.lost_2hop_neighbor = neighbor_timer_summary->lost_2hop_neighbor;
    }

    //if( other ) {
        summary.context_updates = context_updates;
    //}

    DiscoveryAlgorithm* alg = state->args->algorithm;

    bool new_neighbor = summary.new_neighbor;
    bool lost_neighbor = summary.lost_neighbor;
    bool updated_neighbor = summary.updated_neighbor;

    bool hello_is_set = compare_timespec(&state->next_reactive_hello_time, (struct timespec*)&zero_timespec) != 0;
    bool hack_is_set = compare_timespec(&state->next_reactive_hack_time, (struct timespec*)&zero_timespec) != 0;

    struct timespec next_timer = {0};
    if(hello_is_set && !hack_is_set) {
        copy_timespec(&next_timer, &state->next_reactive_hello_time);
    } else if(!hello_is_set && hack_is_set) {
        copy_timespec(&next_timer, &state->next_reactive_hack_time);
    } else if(hello_is_set || hack_is_set) {
        if( compare_timespec(&state->next_reactive_hello_time, &state->next_reactive_hack_time) < 0 ) {
            copy_timespec(&next_timer, &state->next_reactive_hello_time);
        } else {
            copy_timespec(&next_timer, &state->next_reactive_hack_time);
        }
    } else {
        assert(!state->neighbor_change_timer_active);
    }

    struct timespec hello_t = {0};

    if( !hello_is_set ) {
        bool send_hello = \
        (DA_HelloNewNeighbor(alg) && new_neighbor) || \
        (DA_HelloLostNeighbor(alg)  && lost_neighbor) || \
        (DA_HelloUpdateNeighbor(alg) && updated_neighbor) || \
        (DA_HelloContextUpdate(alg) && context_updates);

        if(send_hello) {
            struct timespec* other_timer = hack_is_set ? &state->next_reactive_hack_time : NULL;

            bool set = computeNextTimer(state->args->max_jitter_ms, state->args->min_hello_interval_ms, &state->current_time, &state->last_hello_time, &state->next_hello_time, other_timer, &hello_t);

            if(set) {
                add_timespec(&state->next_reactive_hello_time, &state->current_time, &hello_t);
                hello_is_set = true;
            }
        }
    } else {
        if( compare_timespec(&state->next_reactive_hello_time, &state->current_time) >= 0 ) {
            subtract_timespec(&hello_t, &state->next_reactive_hello_time, &state->current_time);
        } else {
            copy_timespec(&hello_t, &zero_timespec);
        }
    }

    struct timespec hack_t = {0};

    if( !hack_is_set ) {
        bool send_hack =  \
        (DA_HackNewNeighbor(alg) && new_neighbor) || \
        (DA_HackLostNeighbor(alg)  && lost_neighbor) || \
        (DA_HackUpdateNeighbor(alg) && updated_neighbor) || \
        (DA_HackContextUpdate(alg) && context_updates);

        if(send_hack) {
            struct timespec* other_timer = hello_is_set ? &state->next_reactive_hello_time : NULL;

            bool set = computeNextTimer(state->args->max_jitter_ms, state->args->min_hack_interval_ms, &state->current_time, &state->last_hack_time, &state->next_hack_time, other_timer, &hack_t);

            if(set) {
                add_timespec(&state->next_reactive_hack_time, &state->current_time, &hack_t);
                hack_is_set = true;
            }
        }
    } else {
        if( compare_timespec(&state->next_reactive_hack_time, &state->current_time) >= 0 ) {
            subtract_timespec(&hack_t, &state->next_reactive_hack_time, &state->current_time);
        } else {
            copy_timespec(&hack_t, &zero_timespec);
        }
    }

    struct timespec min_t = {0};
    if(hello_is_set && !hack_is_set) {
        copy_timespec(&min_t, &hello_t);
    } else if(!hello_is_set && hack_is_set) {
        copy_timespec(&min_t, &hack_t);
    } else if(hello_is_set || hack_is_set) {
        if( compare_timespec(&hello_t, &hack_t) < 0 ) {
            copy_timespec(&min_t, &hello_t);
        } else {
            copy_timespec(&min_t, &hack_t);
        }
    }

    if(hello_is_set || hack_is_set) {

            //printf("YO hello_is_set = %s (%lu %lu)    hack_is_set = %s (%lu %lu)\n", hello_is_set?"T":"F", hello_t.tv_sec, hello_t.tv_nsec, hack_is_set?"T":"F", hack_t.tv_sec, hack_t.tv_nsec);

            if( !state->neighbor_change_timer_active ) {
                assert(compare_timespec(&next_timer, (struct timespec*)&zero_timespec) == 0);

                // Set timer
                state->neighbor_change_timer_active = true;

                SetTimer(&min_t, state->neighbor_change_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_CHANGE_TIMER);

                //printf("SET TIMER %lu %lu\n", min_t.tv_sec, min_t.tv_nsec);
            } else {
                //assert(compare_timespec(&next_timer, (struct timespec*)&zero_timespec) != 0);

                struct timespec min_next = {0};
                add_timespec(&min_next, &min_t, &state->current_time);

                if(compare_timespec(&min_next, &next_timer) < 0) {
                    // Reset Timer
                    CancelTimer(state->neighbor_change_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID);

                    SetTimer(&min_t, state->neighbor_change_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_CHANGE_TIMER);
                    // printf("RESET TIMER %lu %lu\n", min_t.tv_sec, min_t.tv_nsec);
                }

                //printf("XO\n");
            }
    }
}

HelloDeliverSummary* deliverHello(void* f_state, HelloMessage* hello, WLANAddr* addr, MessageSummary* msg_summary) {

    HelloDeliverSummary* summary = DF_uponHelloMessage((discovery_framework_state*)f_state, hello, addr);
    assert(summary->neigh != NULL);

    assert(msg_summary->hello_summary == NULL);
    msg_summary->hello_summary = newHelloDeliverSummary();
    memcpy(msg_summary->hello_summary, summary, sizeof(HelloDeliverSummary));

    return summary;
}

HackDeliverSummary* deliverHack(void* f_state, HackMessage* hack, MessageSummary* msg_summary) {

    HackDeliverSummary* summary = DF_uponHackMessage((discovery_framework_state*)f_state, hack);

    HackDeliverSummary* summary2 = newHackDeliverSummary();
    memcpy(summary2, summary, sizeof(HackDeliverSummary));
    list_add_item_to_tail(msg_summary->hack_summaries, summary2);

    return summary;
}

HelloDeliverSummary* newHelloDeliverSummary() {
    HelloDeliverSummary* summary = malloc(sizeof(HelloDeliverSummary));

    summary->neigh = NULL;

    summary->became_accepted = false;
    summary->lost_accepted = false;

    summary->new_neighbor = false;
    summary->updated_neighbor = false;
    summary->lost_neighbor = false;
    summary->rebooted = false;
    summary->period_changed = false;
    summary->updated_quality = false;
    summary->updated_quality_threshold = false;
    summary->updated_traffic = false;
    summary->updated_traffic_threshold = false;
    summary->missed_hellos = 0;

    return summary;
}

HackDeliverSummary* newHackDeliverSummary() {
    HackDeliverSummary* summary = malloc(sizeof(HackDeliverSummary));

    summary->neigh = NULL;

    summary->positive_hack = false;
    summary->updated_neighbor = false;
    summary->missed_hacks = 0;
    summary->new_hack = false;
    summary->repeated_yet_fresh_hack = false;
    summary->became_bi = false;
    summary->lost_bi = false;
    summary->period_changed = false;
    summary->updated_quality = false;
    summary->updated_quality_threshold = false;

    summary->updated_2hop_quality = false;
    summary->updated_2hop_quality_threshold = false;
    summary->updated_2hop_traffic = false;
    summary->updated_2hop_traffic_threshold = false;
    summary->became_bi_2hop = false;
    summary->lost_bi_2hop = false;

    summary->updated_2hop_neighbor = false;
    summary->added_2hop_neighbor = false;
    summary->lost_2hop_neighbor = false;

    return summary;
}

NeighborTimerSummary* newNeighborTimerSummary() {
    NeighborTimerSummary* summary = malloc(sizeof(NeighborTimerSummary));

    summary->neigh = NULL;

    summary->became_accepted = false;
    summary->lost_accepted = false;

    summary->updated_neighbor = false;
    summary->lost_neighbor = false;
    summary->removed = false;
    summary->lost_bi = false;
    summary->updated_quality = false;
    summary->updated_quality_threshold = false;
    summary->deleted_2hop = 0;
    summary->missed_hellos = 0;
    summary->missed_hacks = 0;

    return summary;
}

HelloDeliverSummary* DF_uponHelloMessage(discovery_framework_state* state, HelloMessage* hello, WLANAddr* mac_addr) {

    // Log
    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(hello->process_id, id_str);

        char str[200];
        sprintf(str, "SRC=%s SEQ=%hu P=%d s", id_str, hello->seq, hello->period);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "RECEIVED HELLO", str);
    #endif

    HelloDeliverSummary* summary = newHelloDeliverSummary();

    NeighborEntry* neigh = NT_getNeighbor(state->neighbors, hello->process_id);

    struct timespec rx_exp_time;
    milli_to_timespec(&rx_exp_time, hello->period*1000*state->args->hello_misses);
    add_timespec(&rx_exp_time, &rx_exp_time, &state->current_time);

    if( neigh ) {
        int seq_cmp = compare_seq(hello->seq, NE_getNeighborSEQ(neigh), state->args->ignore_zero_seq);
        summary->rebooted = seq_cmp < 0;

        if( NE_isLost(neigh) || summary->rebooted ) {
            flushNeighbor(state, neigh);
            neigh = NULL;

            CancelTimer(hello->process_id, DISCOVERY_FRAMEWORK_PROTO_ID);

            if(summary->rebooted) {
                // Log
                #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, NO_DEBUG)
                    char id_str[UUID_STR_LEN+1];
                    id_str[UUID_STR_LEN] = '\0';
                    uuid_unparse(hello->process_id, id_str);

                    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REBOOTED", id_str);
                #endif
            }
        }
    }

    if( neigh ) {
        unsigned short previous_seq = NE_getNeighborSEQ(neigh);

        int seq_cmp = compare_seq(hello->seq, previous_seq, state->args->ignore_zero_seq);
        assert(seq_cmp >= 0);

        unsigned int prev_missed_hellos = compute_missed(state->args->hello_misses, NE_getNeighborHelloPeriod(neigh)*1000, NE_getNeighborRxExpTime(neigh), NE_getLastNeighborTimer(neigh));

        int missed_hellos = 0;
        if( seq_cmp > 0 ) {
            // Fresh Hello

            unsigned short seq_missed_hellos = seq_cmp - 1;

            if( prev_missed_hellos <= seq_missed_hellos ) {
                // If there were less or equal missed hellos due to timeouts than hellos due to seq difference, then the remaining missed hellos are given by the difference
                missed_hellos = seq_missed_hellos - prev_missed_hellos;
            } else {
                // There were more missed hellos due to timeouts than hellos due to seq difference, which punished the quality more than it should.
                // It is being ignored for now.
                // TODO: what should be done?

                missed_hellos = 0;

                // Log
                #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
                    char str[200];
                    sprintf(str, "prev_missed_hellos=%u   seq_missed_hellos=%d", missed_hellos, seq_missed_hellos);
                    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "HELLO SEQ ERROR", str);
                #endif
            }
        } else {
            // Repeated hello
            missed_hellos = 0;

            // Log
            #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, NO_DEBUG)
                char str[200];
                sprintf(str, "seq=%hu", previous_seq);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REPEATED HELLO SEQ", str);
            #endif
        }
        assert(missed_hellos >= 0);

        summary->missed_hellos = missed_hellos;
        state->stats.missed_hellos += missed_hellos;

        NE_setNeighborRxExpTime(neigh, &rx_exp_time);
        NE_setNeighborSEQ(neigh, hello->seq);

        summary->period_changed = NE_getNeighborHelloPeriod(neigh) != hello->period;
        if( summary->period_changed ) {
            NE_setNeighborHelloPeriod(neigh, hello->period);
        }
    }

    // New Neighbor
    else {
        neigh = newNeighborEntry(mac_addr, hello->process_id, hello->seq, hello->period, hello->traffic, &rx_exp_time, &state->current_time);

        NE_setLinkQualityAttributes(neigh, DA_createLinkQualityAttributes(state->args->algorithm));

        NE_setContextAttributes(neigh, DA_createContextAttributes(state->args->algorithm));

        NT_addNeighbor(state->neighbors, neigh);

        summary->new_neighbor = summary->rebooted ? false : true;
        summary->missed_hellos = 0;

        struct timespec t;
        milli_to_timespec(&t, hello->period*1000);
        SetTimer(&t, hello->process_id, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_TIMER);

        /*if(!summary->rebooted)
            state->stats.new_neighbors++;*/
    }

    // Update Link Quality
    double old_rx_lq = NE_getRxLinkQuality(neigh);
    double new_rx_lq = DA_computeLinkQuality(state->args->algorithm, NE_getLinkQualityAttributes(neigh), old_rx_lq, 1, summary->missed_hellos, summary->new_neighbor, &state->current_time);
    new_rx_lq = roundPrecision(new_rx_lq, state->args->lq_precision);

    double lq_delta = fabs(old_rx_lq - new_rx_lq);
    if( lq_delta >= state->args->lq_epsilon || (lq_delta > 0 && (new_rx_lq == 1.0 || new_rx_lq == 0.0)) ) {
        NE_setRxLinkQuality(neigh, new_rx_lq);
        summary->updated_quality = true;

        if( lq_delta >= state->args->lq_threshold ) {
            summary->updated_quality_threshold = true;
        }
    }

    // Update Link Admission
    bool old_accepted = NE_isAccepted(neigh);
    bool accepted = DA_evalLinkAdmission(state->args->algorithm, neigh, &state->current_time);
    NE_setAccepted(neigh, accepted);

    summary->became_accepted = !old_accepted && accepted;
    summary->lost_accepted = old_accepted && !accepted;

    if(summary->became_accepted) {
        if( NE_isPending(neigh) ) {
            NE_setPending(neigh, false);
            summary->new_neighbor = true;
        } else {
            assert(false); // Error
        }
    }

    if(summary->lost_accepted) {
        assert(!NE_isPending(neigh));

        // Neighbor is dead
        if( !NE_isLost(neigh) ) {
            NE_setLost(neigh, &state->current_time);

            state->stats.lost_neighbors++;

            summary->lost_neighbor = true;

            struct timespec removal_time;
            milli_to_timespec(&removal_time, state->args->neigh_hold_time_s*1000);
            add_timespec(&removal_time, &removal_time,  &state->current_time);
            NE_setNeighborRemovalTime(neigh, &removal_time);
        } else {
            assert(false); // Error
        }
    }

    // Update traffic
    double traffic_delta = fabs(NE_getOutTraffic(neigh) - hello->traffic);
    if( traffic_delta >= state->args->traffic_epsilon || (traffic_delta > 0 && hello->traffic == 0.0) ) {
        NE_setOutTraffic(neigh, hello->traffic);
        summary->updated_traffic = true && state->args->toggle_env;

        if( traffic_delta >= state->args->traffic_threshold ) {
            summary->updated_traffic_threshold = true && state->args->toggle_env;
        }
    }

    /*

    // Re-schedule neighbor timer
    if( summary->period_changed ) {
        scheduleNeighborTimer(state, neigh);
    }

    // Notify
    if( summary->new_neighbor ) {
        // DF_notifyNewNeighbor(state, neigh);
        assert(state->dirty_neighbor == NULL || state->dirty_neighbor == neigh);
        state->dirty_neighbor = neigh;
        state->dirty_new_neighbor = true;

        scheduleNeighborChange(state, summary, NULL, NULL, false);

        DE_registerNewNeighbor(state->environment, &state->current_time);
    } else if( summary->updated_quality || summary->updated_traffic || summary->rebooted ) {
        summary->updated_neighbor = true;
        // DF_notifyUpdateNeighbor(state, neigh);
        assert(state->dirty_neighbor == NULL || state->dirty_neighbor == neigh);
        state->dirty_neighbor = neigh;
        state->dirty_update_neighbor = true;

        if( summary->updated_quality_threshold || summary->updated_traffic_threshold || summary->rebooted ) {
            scheduleNeighborChange(state, summary, NULL, NULL, false);
        }
    }
    */

    if( summary->updated_quality || summary->updated_traffic || summary->rebooted ) {
        summary->updated_neighbor = true;
    }

    summary->neigh = neigh;

    if(!NE_isPending(neigh)) {
        scheduleReply(state, hello, summary);
    }

    return summary;
}

HackDeliverSummary* DF_uponHackMessage(discovery_framework_state* state, HackMessage* hack) {

    bool positive_hack = hack->neigh_type != LOST_NEIGH;

    // Log
    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
        char id_str1[UUID_STR_LEN+1];
        id_str1[UUID_STR_LEN] = '\0';
        uuid_unparse(hack->src_process_id, id_str1);

        char id_str2[UUID_STR_LEN+1];
        id_str2[UUID_STR_LEN] = '\0';
        uuid_unparse(hack->dest_process_id, id_str2);

        char str[200];
        sprintf(str, "SRC=%s DEST=%s SEQ=%hu P=%d s [%s]", id_str1, id_str2, hack->seq, hack->period, (positive_hack?"+":"-"));
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "RECEIVED HACK", str);
    #endif

    HackDeliverSummary* summary = newHackDeliverSummary();

    NeighborEntry* neigh = NT_getNeighbor(state->neighbors, hack->src_process_id);

    // The neighbor is known already and is not dead
    if( neigh && !NE_isLost(neigh) && !NE_isPending(neigh) ) {
            summary->neigh = neigh;

            assert(!NE_isPending(neigh));

            // Compute the expiration time for this hack
            struct timespec hack_exp_time;
            milli_to_timespec(&hack_exp_time, hack->period*1000*state->args->hack_misses);
            add_timespec(&hack_exp_time, &hack_exp_time, &state->current_time);


            summary->positive_hack = positive_hack;

            // If is a positive HACK
            if( positive_hack ) {

                // Check if the hack is meant for me
                if( uuid_compare(state->myID, hack->dest_process_id) == 0 ) {

                    struct timespec* tx_exp_time = NE_getNeighborTxExpTime(neigh);

                    unsigned int prev_missed_hacks = 0;
                    int missed_hacks = 0;

                    bool first_hack = compare_timespec(tx_exp_time, (struct timespec*)&zero_timespec) == 0;
                    bool expired = compare_timespec(tx_exp_time, &state->current_time) <= 0;

                    if(first_hack) {
                        summary->new_hack = true;
                        summary->repeated_yet_fresh_hack = false;
                        prev_missed_hacks = 0;
                        missed_hacks = 0;
                    } else if(expired) {
                        // Check freshness of the hack
                        int seq_cmp = compare_seq(hack->seq, NE_getNeighborHSEQ(neigh), state->args->ignore_zero_seq);
                        int seq_cmp2 = compare_seq(hack->seq, dec_seq(state->my_seq, state->args->ignore_zero_seq), state->args->ignore_zero_seq);

                        // If my hello period is larger than my neighbor's hack period and if several hacks are missed, then it may happen that this hack is repeated_yet_fresh (valid)

                        summary->new_hack = seq_cmp > 0;
                        summary->repeated_yet_fresh_hack = seq_cmp == 0 && seq_cmp2 >= 0;
                        prev_missed_hacks = 0;
                        missed_hacks = 0;
                    } else { // !first_hack && !expired
                        // Check freshness of the hack
                        unsigned short previous_seq = NE_getNeighborHSEQ(neigh);
                        int seq_cmp = compare_seq(hack->seq, previous_seq, state->args->ignore_zero_seq);
                        int seq_cmp2 = compare_seq(hack->seq, dec_seq(state->my_seq, state->args->ignore_zero_seq), state->args->ignore_zero_seq);

                        summary->new_hack = seq_cmp > 0;
                        summary->repeated_yet_fresh_hack = seq_cmp == 0 && seq_cmp2 >= 0;

                        // Compute previously missed hacks, from the reception of the last valid hack until the last neighbor timer
                        prev_missed_hacks = compute_missed(state->args->hack_misses, NE_getNeighborHackPeriod(neigh)*1000, NE_getNeighborTxExpTime(neigh), NE_getLastNeighborTimer(neigh));

                        // If there is a difference between the hack's seq and the previously received seq, then hack misses occurred
                        if( seq_cmp > 0 ) {
                            // If there are less or equal amount of prev_missed_hacks (due to timer) than real missed not repeated hacks (due to seq difference), the amount of missed hacks that was not counted yet corresponds to the difference between the two amounts
                            if( prev_missed_hacks <= seq_cmp ) {
                                missed_hacks = seq_cmp - prev_missed_hacks;
                            } else {
                                // In case the amount of prev_missed_hacks (due to timer) is larger than real missed not repeated hacks (due to seq diference), the link quality was punished more than it should have. This may happen if the neighbor increases its hack period and the current node does not become aware of the new larger period (due to missing all the hacks with the new period until now).
                                // It may also occur if the neighbor sends its hack after the expiration time (neighbor is slow or congestioned with events)
                                // For now, this is being treated as if no new miss happenend, ignoring the "extra" punishment that the link quality received.
                                // TODO: what should be done in this case?

                                missed_hacks = 0;

                                //assert(hack->period > NE_getNeighborHackPeriod(neigh));

                                // Log
                                #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, NO_DEBUG)
                                    char str[200];
                                    sprintf(str, "prev_missed_hacks=%u   seq_cmp-1=%d", missed_hacks, seq_cmp-1);
                                    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "HACK SEQ ERROR", str);
                                #endif
                            }
                        } else {
                            missed_hacks = 0;
                        }
                    }
                    assert( missed_hacks >= 0 );

                    summary->missed_hacks = missed_hacks;
                    state->stats.missed_hacks += missed_hacks;

                    if( summary->new_hack || summary->repeated_yet_fresh_hack ) {

                        // Update the last received seq on a hack
                        NE_setNeighborHSEQ(neigh, hack->seq);

                        // Check if the neighbor became symmetric (or bi)
                        summary->became_bi = NE_getNeighborType(neigh, &state->current_time) != BI_NEIGH;
                        if( summary->became_bi ) {
                            // Log
                            #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
                                char id_str[UUID_STR_LEN+1];
                                id_str[UUID_STR_LEN] = '\0';
                                uuid_unparse(hack->src_process_id, id_str);

                                char str[200];
                                sprintf(str, "%s", id_str);
                                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "BECAME BI", str);
                            #endif

                            assert(!NE_isPending(neigh));
                        }

                        // Update Tx expiration timestamp
                        NE_setNeighborTxExpTime(neigh, &hack_exp_time);

                        // Update Link Quality
                        double old_tx_lq = NE_getTxLinkQuality(neigh);
                        double new_tx_lq = hack->rx_lq;

                        double lq_delta = fabs(old_tx_lq - new_tx_lq);
                        if( lq_delta >= state->args->lq_epsilon || (lq_delta > 0 && (new_tx_lq == 1.0 || new_tx_lq == 0.0)) ) {

                            NE_setTxLinkQuality(neigh, new_tx_lq);
                            summary->updated_quality = true;

                            if( lq_delta >= state->args->lq_threshold ) {

                                summary->updated_quality_threshold = true;
                            }
                        }

                        // Check if HACK period changed
                        if( NE_getNeighborHackPeriod(neigh) != hack->period ) {
                            NE_setNeighborHackPeriod(neigh, hack->period);

                            summary->period_changed = true;
                        }
                    } else {
                        // stale hack, ignore
                    }
                }

                // Update 2-hop neighborhood (Neighbor's neighbors)
                TwoHopNeighborEntry* nn = NE_getTwoHopNeighborEntry(neigh, hack->dest_process_id);
                if( nn ) {
                    // Update two-hop neighbor

                    // Check freshness of the hack
                    int seq_cmp = compare_seq(hack->seq, THNE_getHSEQ(nn), state->args->ignore_zero_seq);
                    int seq_cmp2 = compare_seq(hack->seq, dec_seq(NE_getNeighborSEQ(neigh), state->args->ignore_zero_seq), state->args->ignore_zero_seq);

                    // If my hello period is larger than my neighbor's hack period and if several hacks are missed, then it may happen that this hack is repeated_yet_fresh (valid)

                    summary->new_hack = seq_cmp > 0;
                    summary->repeated_yet_fresh_hack = seq_cmp == 0 && seq_cmp2 >= 0 && !THNE_isLost(nn);

                    // if fresh hack
                    if( summary->new_hack || summary->repeated_yet_fresh_hack ) {
                        THNE_setHSEQ(nn, hack->seq);

                        bool is_bi = hack->neigh_type == BI_NEIGH;
                        if( THNE_isBi(nn) != is_bi ) {
                            summary->became_bi_2hop = is_bi && !THNE_isBi(nn);
                            summary->lost_bi_2hop = !is_bi && THNE_isBi(nn);

                            THNE_setBi(nn, is_bi);
                        }

                        double rx_lq_delta = fabs(THNE_getRxLinkQuality(nn) - hack->rx_lq);
                        double tx_lq_delta = fabs(THNE_getTxLinkQuality(nn) - hack->tx_lq);
                        if( rx_lq_delta >= state->args->lq_epsilon || tx_lq_delta >= state->args->lq_epsilon || (rx_lq_delta > 0.0 && (hack->rx_lq == 1.0 || hack->rx_lq == 0.0)) || (tx_lq_delta > 0 && (hack->tx_lq == 1.0 || hack->tx_lq == 0.0))) {

                            THNE_setRxLinkQuality(nn, hack->rx_lq);
                            THNE_setTxLinkQuality(nn, hack->tx_lq);

                            summary->updated_2hop_quality = true;

                            if( rx_lq_delta >= state->args->lq_threshold || tx_lq_delta >= state->args->lq_threshold ) {
                                summary->updated_2hop_quality_threshold = true;
                            }
                        }

                        double traffic_delta = fabs(THNE_getTraffic(nn) - hack->traffic);
                        if( traffic_delta >= state->args->traffic_epsilon || (traffic_delta > 0 && hack->traffic == 0.0)) {

                            THNE_setTraffic(nn, hack->traffic);
                            summary->updated_2hop_traffic = true && state->args->toggle_env;

                            if( traffic_delta >= state->args->traffic_threshold ) {
                                summary->updated_2hop_traffic_threshold = true && state->args->toggle_env;
                            }
                        }

                        // To avoid deleting before the neighbor deletes this 2hop neigh, which could lead to the 1hop neighbor sending a positive hack after the current node deleting
                        /*struct timespec aux = {0};
                        milli_to_timespec(&aux, hack->period*1000);
                        add_timespec(&aux, &aux, &hack_exp_time);
                        THNE_setExpiration(nn, &aux);*/
                        THNE_setExpiration(nn, &hack_exp_time);
                    } else {
                        // stale hack, ignore
                    }

                } else {
                    // Insert new 2-hop neighbor
                    TwoHopNeighborEntry* nn = newTwoHopNeighborEntry(hack->dest_process_id, hack->seq, hack->neigh_type == BI_NEIGH, false, hack->rx_lq, hack->tx_lq, hack->traffic, &hack_exp_time);

                    TwoHopNeighborEntry* aux = NE_addTwoHopNeighborEntry(neigh, nn);
                    assert(aux == NULL);

                    summary->added_2hop_neighbor = true;
                }
            }

            // TODO: check below here

            // If is a negative HACK
            else {
                // Since it is a Negative HACK, then the neighbor does not receive hellos from the current node for a long time (thus considering the current node as failed). Therefore, the NHACKS's SEQ may be repeated. In case some previous hacks were missed, the NHACKS's SEQ may even be new than the previously received SEQ.

                // Check if the hack is meant for me
                if( uuid_compare(state->myID, hack->dest_process_id) == 0 ) {
                    // Check freshness of the hack
                    int seq_cmp = compare_seq(hack->seq, NE_getNeighborHSEQ(neigh), state->args->ignore_zero_seq);
                    int seq_cmp2 = compare_seq(hack->seq, dec_seq(state->my_seq, state->args->ignore_zero_seq), state->args->ignore_zero_seq);

                    summary->new_hack = seq_cmp > 0;
                    summary->repeated_yet_fresh_hack = seq_cmp == 0 && seq_cmp == 0 && seq_cmp2 >= 0;

                    if( summary->new_hack || summary->repeated_yet_fresh_hack ) {
                        NE_setNeighborHSEQ(neigh, hack->seq);

                        // Lost BI
                        NE_setNeighborTxExpTime(neigh, &state->current_time);
                        NE_setTxLinkQuality(neigh, 0.0);
                        summary->lost_bi = true;

                        // Log
                        #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
                            char id_str[UUID_STR_LEN+1];
                            id_str[UUID_STR_LEN] = '\0';
                            uuid_unparse(hack->src_process_id, id_str);

                            char str[200];
                            sprintf(str, "%s", id_str);
                            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "LOST BI", str);
                        #endif
                    }
                }

                struct timespec removal_time = {0};
                milli_to_timespec(&removal_time, state->args->neigh_hold_time_s*1000);
                add_timespec(&removal_time, &removal_time,  &state->current_time);

                // Update 2-hop neighborhood
                TwoHopNeighborEntry* nn = NE_getTwoHopNeighborEntry(neigh, hack->dest_process_id);
                if( nn ) {
                    // Check freshness of the hack
                    int seq_cmp = compare_seq(hack->seq, THNE_getHSEQ(nn), state->args->ignore_zero_seq);
                    int seq_cmp2 = compare_seq(hack->seq, dec_seq(NE_getNeighborSEQ(neigh), state->args->ignore_zero_seq), state->args->ignore_zero_seq);

                    // If my hello period is larger than my neighbor's hack period and if several hacks are missed, then it may happen that this hack is repeated_yet_fresh (valid)

                    summary->new_hack = seq_cmp > 0;
                    summary->repeated_yet_fresh_hack = seq_cmp == 0 && seq_cmp2 >= 0;

                    // if fresh hack
                    if( summary->new_hack || summary->repeated_yet_fresh_hack ) {
                        if( !THNE_isLost(nn) ) {
                            THNE_setLost(nn, true);
                            THNE_setExpiration(nn, &removal_time);
                        }

                        // Remove from 2-hop neighborhood, if present
                        /*TwoHopNeighborEntry* removed = NE_removeTwoHopNeighborEntry(neigh, hack->dest_process_id);
                        if(removed) {
                            free(removed);
                            summary->lost_2hop_neighbor = true;
                        }*/
                    }
                } else {
                    // Insert new 2-hop neighbor
                    nn = newTwoHopNeighborEntry(hack->dest_process_id, hack->seq, false, true, hack->rx_lq, hack->tx_lq, hack->traffic, &removal_time);

                    TwoHopNeighborEntry* aux = NE_addTwoHopNeighborEntry(neigh, nn);
                    assert(aux == NULL);
                }
            }
    } else {
        // The neighbor is not know. Ignore the HACK
    }

    summary->updated_neighbor = summary->became_bi || summary->lost_bi || summary->updated_quality;
    summary->updated_2hop_neighbor = summary->became_bi_2hop || summary->lost_bi_2hop || summary->updated_2hop_quality || summary->updated_2hop_traffic;

    /*
if( summary->updated_neighbor || summary->updated_2hop_neighbor || summary->added_2hop_neighbor || summary->lost_2hop_neighbor ) {
        //DF_notifyUpdateNeighbor(state, neigh);
        DF_setNotifyUpdateNeighbor(state, neigh);

        if( summary->became_bi || summary->lost_bi || summary->updated_quality_threshold || (summary->became_bi_2hop || summary->lost_bi_2hop || summary->updated_2hop_quality_threshold || summary->updated_2hop_traffic_threshold) || summary->added_2hop_neighbor || summary->lost_2hop_neighbor ) {
            scheduleNeighborChange(state, NULL, summary, NULL, false);
        }
    }
*/



    // Re-schedule neighbor timer
    // scheduleNeighborTimer(state, neigh);

    return summary;
}

/*
void DF_setNotifyNewNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);


}

void DF_setNotifyUpdateNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    assert(state->dirty_neighbor == NULL || state->dirty_neighbor == neigh);
    state->dirty_neighbor = neigh;
    state->dirty_update_neighbor = true;
}
*/



/*

bool DF_createMessage(discovery_framework_state* state, YggMessage* msg, HelloMessage* hello, HackMessage* hacks, byte n_hacks, WLANAddr* addr, MessageType msg_type, void* aux_info) {
    assert(msg);

    // Serialize Message
    byte buffer[YGG_MESSAGE_PAYLOAD];
    unsigned short buffer_size = 0;

    bool send = false;
    if( hello || hacks ) {
        bool result = DA_createDiscoveryMessage(state->args->algorithm, state->myID, &state->current_time, state->neighbors, msg_type, aux_info, hello, hacks, n_hacks, buffer + sizeof(buffer_size), &buffer_size);

        memcpy(buffer, &buffer_size, sizeof(buffer_size));

        // only non periodic nor reply msgs can be canceled
        send = (msg_type == PERIODIC_MSG || msg_type == REPLY_MSG) ? true : result;
    }

    if(send) {
        assert(hello || n_hacks > 0);

        pushPayload(msg, (char*)buffer, sizeof(buffer_size) + buffer_size, DISCOVERY_FRAMEWORK_PROTO_ID, addr);

        if( hello ) {
            copy_timespec(&state->last_hello_time, &state->current_time);

            // Re-schedule
            scheduleHelloTimer(state, false);

            state->stats.total_hellos++;
        }

        if( hacks ) {
            copy_timespec(&state->last_hack_time, &state->current_time);

            WLANAddr* bcast_addr = getBroadcastAddr();
            bool not_unicast = memcmp(addr->data, bcast_addr, WLAN_ADDR_LEN) == 0;
            free(bcast_addr);

            if( not_unicast ) {
                // Re-schedule
                scheduleHackTimer(state, false);
            }

            state->stats.total_hacks += n_hacks;
        }

        //ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REGISTER TRAFFIC", "");
        DE_registerOutTraffic(state->environment, &state->current_time);

    } else {
        buffer_size = 0;

        pushPayload(msg, (char*)&buffer_size, sizeof(buffer_size), DISCOVERY_FRAMEWORK_PROTO_ID, addr);

        if(hello) {
            // Decrement SEQ
            state->my_seq = dec_seq(state->my_seq, state->args->ignore_zero_seq);
        }
    }

    return send;
}


*/



void DF_piggybackDiscovery(discovery_framework_state* state, YggMessage* msg) {

    // TODO: try to leverage promiscuous mode in the future

    DiscoverySendPack* dsp = DF_handleDiscoveryEvent(state, DPE_DOWNSTREAM_MESSAGE, msg, msg);

    if( dsp ) {
        #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
        char hello_str[10];
        if( dsp->hello ) {
            sprintf(hello_str, "SEQ=%hu", dsp->hello->seq);
        } else {
            sprintf(hello_str, "-");
        }

        char hack_str[10];
        if( dsp->n_hacks > 0 ) {
            sprintf(hack_str, "%d", dsp->n_hacks);
        } else {
            sprintf(hack_str, "-");
        }

        const char* addr_str = is_unicast_message(msg) ? "UNICAST" : "BROADCAST";

        char str[200];
        sprintf(str, "HELLO=[%s] HACK=[%s] [%s]", hello_str, hack_str, addr_str);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "PIGGYBACK MESSAGE", str);
        #endif

        if( dsp->hello ) {
            state->stats.piggybacked_hellos++;
            state->stats.total_hellos++;

            free(dsp->hello);
        }

        if( dsp->hacks ) {
            state->stats.piggybacked_hacks += dsp->n_hacks;
            state->stats.total_hacks += dsp->n_hacks;

            free(dsp->hacks);
        }

        free(dsp);
    }

}

/*
void changeAlgorithm(discovery_framework_state* state, DiscoveryAlgorithm* new_alg) {
    assert( new_alg );

    if( state->hello_timer_active ) {
        CancelTimer(state->hello_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID);

        state->hello_timer_active = false;
    }

    if( state->hack_timer_active ) {
        CancelTimer(state->hack_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID);

        state->hack_timer_active = false;
    }

    DiscoveryAlgorithm* old_alg = state->args->algorithm;

    destroyDiscoveryAlgorithm(old_alg);

    state->args->algorithm = new_alg;

    scheduleHelloTimer(state, true);
    scheduleHackTimer(state, true);

    // Log
    #ifdef DEBUG_DISCOVERY
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "CHANGED ALGORITHM", "");
    #endif
}
*/


void DF_printNeighbors(discovery_framework_state* state) {
    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
    //char* str1 = NULL, *str2 = NULL;
    char* str2 = NULL;

    //DF_uponDiscoveryEnvironmentTimer(state); // temp
    //NE_print(state->environment, &str1);

    NT_print(state->neighbors, &str2, &state->current_time, state->myID, &state->myAddr, state->my_seq);

    //char str3[strlen(str1)+strlen(str2)+1];
    //sprintf(str3, "\n%s\n%s", str1, str2);

    char str3[strlen(str2)+1];
    sprintf(str3, "\n%s", str2);

    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBORHOOD", str3);

    //free(str1);
    free(str2);
    #endif
}

void DF_uponStatsRequest(discovery_framework_state* state, YggRequest* req) {
    unsigned short dest = req->proto_origin;
	YggRequest_init(req, DISCOVERY_FRAMEWORK_PROTO_ID, dest, REPLY, REQ_DISCOVERY_FRAMEWORK_STATS);

	YggRequest_addPayload(req, (void*)&state->stats, sizeof(discovery_stats));

	deliverReply(req);

	YggRequest_freePayload(req);
}

void DF_notifyDiscoveryEnvironment(discovery_framework_state* state) {
    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, DISCOVERY_ENVIRONMENT_UPDATE);

    double in_traffic = DE_getInTraffic(state->environment);
    YggEvent_addPayload(ev, &in_traffic, sizeof(in_traffic));

    double out_traffic = DE_getOutTraffic(state->environment);
    YggEvent_addPayload(ev, &out_traffic, sizeof(out_traffic));

    double new_neighbors_flux = DE_getNewNeighborsFlux(state->environment);
    YggEvent_addPayload(ev, &new_neighbors_flux, sizeof(new_neighbors_flux));

    double lost_neighbors_flux = DE_getLostNeighborsFlux(state->environment);
    YggEvent_addPayload(ev, &lost_neighbors_flux, sizeof(lost_neighbors_flux));

    unsigned int n_neighbors = DE_getNNeighbors(state->environment);
    YggEvent_addPayload(ev, &n_neighbors, sizeof(n_neighbors));

    double neighbors_density = DE_getNeigbhorsDensity(state->environment);
    YggEvent_addPayload(ev, &neighbors_density, sizeof(neighbors_density));

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}

void DF_notifyNewNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    YggEvent ev = {0};
    YggEvent_init(&ev, DISCOVERY_FRAMEWORK_PROTO_ID, NEW_NEIGHBOR);

    // Append Neighbor ID
    YggEvent_addPayload(&ev, NE_getNeighborID(neigh), sizeof(uuid_t));

    // Append MAC addr
    YggEvent_addPayload(&ev, NE_getNeighborMAC(neigh)->data, WLAN_ADDR_LEN);

    // Append LQs
    double rx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(&ev, &rx_lq, sizeof(double));
    double tx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(&ev, &tx_lq, sizeof(double));

    // Append Traffic
    double traffic = NE_getOutTraffic(neigh);
    YggEvent_addPayload(&ev, &traffic, sizeof(double));

    // Append Neighbor Type
    byte is_bi = NE_getNeighborType(neigh, &state->current_time) == BI_NEIGH;
    YggEvent_addPayload(&ev, &is_bi, sizeof(byte));

    assert(!NE_isPending(neigh));

    // Append Neighbors
    hash_table* ht = NE_getTwoHopNeighbors(neigh);
    byte n = 0;

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;
        if(!THNE_isLost(nn)) {
            n++;
        }
    }

    YggEvent_addPayload(&ev, &n, sizeof(byte));

    iterator = NULL;
    hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;

        if(!THNE_isLost(nn)) {
            // Append Neigh ID
            YggEvent_addPayload(&ev, THNE_getID(nn), sizeof(uuid_t));

            // Append Neigh LQ
            rx_lq = THNE_getRxLinkQuality(nn);
            YggEvent_addPayload(&ev, &rx_lq, sizeof(double));
            tx_lq = THNE_getTxLinkQuality(nn);
            YggEvent_addPayload(&ev, &tx_lq, sizeof(double));

            // Append Traffic
            traffic = THNE_getTraffic(nn);
            YggEvent_addPayload(&ev, &traffic, sizeof(double));

            // Append Neigh Type
            is_bi = THNE_isBi(nn);
            YggEvent_addPayload(&ev, &is_bi, sizeof(byte));
        }
    }

    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(NE_getNeighborID(neigh), id_str);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEW NEIGHBOR", id_str);
    #endif

    YggEvent packet = {0};
    YggEvent_init(&packet, DISCOVERY_FRAMEWORK_PROTO_ID, NEW_NEIGHBOR);

    YggEvent_addPayload(&packet, &ev.length, sizeof(unsigned short));
    YggEvent_addPayload(&packet, ev.payload, ev.length);
    YggEvent_freePayload(&ev);

    YggEvent* neighborhood = DF_notifyNeighborhood(state);
    YggEvent_addPayload(&packet, &neighborhood->length, sizeof(unsigned short));
    YggEvent_addPayload(&packet, neighborhood->payload, neighborhood->length);
    YggEvent_freePayload(neighborhood);
    free(neighborhood);

    YggEvent* x = NULL;
    while( (x = list_remove_head(state->pending_notifications)) ) {
        YggEvent_addPayload(&packet, &x->length, sizeof(unsigned short));
        YggEvent_addPayload(&packet, x->payload, x->length);
        YggEvent_freePayload(x);
        free(x);
    }

    deliverEvent(&packet);
    YggEvent_freePayload(&packet);
}

void DF_notifyUpdateNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    YggEvent ev = {0};
    YggEvent_init(&ev, DISCOVERY_FRAMEWORK_PROTO_ID, UPDATE_NEIGHBOR);

    // Append Neighbor ID
    YggEvent_addPayload(&ev, NE_getNeighborID(neigh), sizeof(uuid_t));

    // Append MAC addr
    YggEvent_addPayload(&ev, NE_getNeighborMAC(neigh)->data, WLAN_ADDR_LEN);

    // Append LQs
    double rx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(&ev, &rx_lq, sizeof(double));
    double tx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(&ev, &tx_lq, sizeof(double));

    // Append Traffic
    double traffic = NE_getOutTraffic(neigh);
    YggEvent_addPayload(&ev, &traffic, sizeof(double));

    // Append Neighbor Type
    byte is_bi = NE_getNeighborType(neigh, &state->current_time) == BI_NEIGH;
    YggEvent_addPayload(&ev, &is_bi, sizeof(byte));

    assert(!NE_isPending(neigh));

    // Append Neighbors
    hash_table* ht = NE_getTwoHopNeighbors(neigh);
    byte n = 0;

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;
        if(!THNE_isLost(nn)) {
            n++;
        }
    }

    YggEvent_addPayload(&ev, &n, sizeof(byte));

    iterator = NULL;
    hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;

        if(!THNE_isLost(nn)) {
            // Append Neigh ID
            YggEvent_addPayload(&ev, THNE_getID(nn), sizeof(uuid_t));

            // Append Neigh LQ
            rx_lq = THNE_getRxLinkQuality(nn);
            YggEvent_addPayload(&ev, &rx_lq, sizeof(double));
            tx_lq = THNE_getTxLinkQuality(nn);
            YggEvent_addPayload(&ev, &tx_lq, sizeof(double));

            // Append Traffic
            traffic = THNE_getTraffic(nn);
            YggEvent_addPayload(&ev, &traffic, sizeof(double));

            // Append Neigh Type
            is_bi = THNE_isBi(nn);
            YggEvent_addPayload(&ev, &is_bi, sizeof(byte));
        }
    }

    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(NE_getNeighborID(neigh), id_str);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "UPDATE NEIGHBOR", id_str);
    #endif

    YggEvent packet = {0};
    YggEvent_init(&packet, DISCOVERY_FRAMEWORK_PROTO_ID, UPDATE_NEIGHBOR);

    YggEvent_addPayload(&packet, &ev.length, sizeof(unsigned short));
    YggEvent_addPayload(&packet, ev.payload, ev.length);
    YggEvent_freePayload(&ev);

    YggEvent* neighborhood = DF_notifyNeighborhood(state);
    YggEvent_addPayload(&packet, &neighborhood->length, sizeof(unsigned short));
    YggEvent_addPayload(&packet, neighborhood->payload, neighborhood->length);
    YggEvent_freePayload(neighborhood);
    free(neighborhood);

    YggEvent* x = NULL;
    while( (x = list_remove_head(state->pending_notifications)) ) {
        YggEvent_addPayload(&packet, &x->length, sizeof(unsigned short));
        YggEvent_addPayload(&packet, x->payload, x->length);
        YggEvent_freePayload(x);
        free(x);
    }

    deliverEvent(&packet);
    YggEvent_freePayload(&packet);
}

void DF_notifyLostNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    YggEvent ev = {0};
    YggEvent_init(&ev, DISCOVERY_FRAMEWORK_PROTO_ID, LOST_NEIGHBOR);

    // Append Neighbor ID
    YggEvent_addPayload(&ev, NE_getNeighborID(neigh), sizeof(uuid_t));

    // Append Neighbor Type
    struct timespec aux;
    copy_timespec(&aux, &state->current_time);
    aux.tv_sec--;
    byte is_bi = NE_getNeighborType(neigh, &aux) == BI_NEIGH;
    YggEvent_addPayload(&ev, &is_bi, sizeof(byte));

    assert(!NE_isPending(neigh));

    // Append Neighbors

    // Append Neighbors
    hash_table* ht = NE_getTwoHopNeighbors(neigh);
    byte n = 0;

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;
        if(!THNE_isLost(nn)) {
            n++;
        }
    }

    YggEvent_addPayload(&ev, &n, sizeof(byte));

    iterator = NULL;
    hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;

        if(!THNE_isLost(nn)) {
            // Append Neigh ID
            YggEvent_addPayload(&ev, THNE_getID(nn), sizeof(uuid_t));

            // Append Neigh Type
            is_bi = THNE_isBi(nn);
            YggEvent_addPayload(&ev, &is_bi, sizeof(byte));
        }
    }

    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(NE_getNeighborID(neigh), id_str);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "LOST NEIGHBOR", id_str);
    #endif

    YggEvent packet = {0};
    YggEvent_init(&packet, DISCOVERY_FRAMEWORK_PROTO_ID, LOST_NEIGHBOR);

    YggEvent_addPayload(&packet, &ev.length, sizeof(unsigned short));
    YggEvent_addPayload(&packet, ev.payload, ev.length);
    YggEvent_freePayload(&ev);

    YggEvent* neighborhood = DF_notifyNeighborhood(state);
    YggEvent_addPayload(&packet, &neighborhood->length, sizeof(unsigned short));
    YggEvent_addPayload(&packet, neighborhood->payload, neighborhood->length);
    YggEvent_freePayload(neighborhood);
    free(neighborhood);

    YggEvent* x = NULL;
    while( (x = list_remove_head(state->pending_notifications)) ) {
        YggEvent_addPayload(&packet, &x->length, sizeof(unsigned short));
        YggEvent_addPayload(&packet, x->payload, x->length);
        YggEvent_freePayload(x);
        free(x);
    }

    deliverEvent(&packet);
    YggEvent_freePayload(&packet);
}

YggEvent* DF_notifyNeighborhood(discovery_framework_state* state) {

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, 0);

    // Append Neighborhood
    unsigned int size = 0;
    byte* buffer = NULL;
    double out_traffic = DE_getOutTraffic(state->environment);
    NT_serialize(state->neighbors, state->myID, &state->myAddr, out_traffic, &state->current_time, &buffer, &size);
    YggEvent_addPayload(ev, buffer, size);
    free(buffer);

    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
    char* str1 = NULL, *str2 = NULL;

    if(state->args->toggle_env) {
        DF_uponDiscoveryEnvironmentTimer(state);
        NE_print(state->environment, &str1);
    } else {
        str1 = "";
    }

    NT_print(state->neighbors, &str2, &state->current_time, state->myID, &state->myAddr, state->my_seq);

    char str3[strlen(str1)+strlen(str2)+1];
    sprintf(str3, "\n%s\n%s", str1, str2);

    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBORHOOD", str3);

    if(state->args->toggle_env) {
        free(str1);
    }

    free(str2);
    #endif

    /*deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);*/
    return ev;
}

void DF_notifyGenericEvent(void* f_state, char* type, void* buffer, unsigned int size) {
    assert(type);

    discovery_framework_state* state = (discovery_framework_state*)f_state;

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, 0);

    // Append type
    unsigned int str_len = strlen(type);
    YggEvent_addPayload(ev, &str_len, sizeof(unsigned int));
    YggEvent_addPayload(ev, type, str_len);

    // Append Payload
    YggEvent_addPayload(ev, buffer, size);

    #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
        char str[30];
        sprintf(str, "[%s]", type);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "GENERIC EVENT", str);
    #endif

    list_add_item_to_tail(state->pending_notifications, ev);

    //deliverEvent(ev);
    //YggEvent_freePayload(ev);
    //free(ev);
}


MessageSummary* newMessageSummary() {
    MessageSummary* summary = malloc(sizeof(MessageSummary));

    //summary->neigh = NULL;
    summary->hello_summary = NULL;
    summary->hack_summaries = list_init();

    return summary;
}


















DiscoverySendPack* DF_triggerDiscoveryEvent(discovery_framework_state* state, DiscoveryInternalEventType event_type, void* event_args, YggMessage* msg) {

    YggMessage_initBcast(msg, DISCOVERY_FRAMEWORK_PROTO_ID);
    DiscoverySendPack* dsp = DF_handleDiscoveryEvent(state, event_type, event_args, msg);

    return dsp;
}

void DF_sendMessage(discovery_framework_state* state, DiscoverySendPack* dsp, YggMessage* msg) {

    if(dsp) {
        // Insert into dispatcher queue
        DF_dispatchMessage(state->dispatcher_queue, msg);

        state->stats.discovery_messages++;

        #if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, SIMPLE_DEBUG)
        char hello_str[10];
        if( dsp->hello ) {
            sprintf(hello_str, "SEQ=%hu", dsp->hello->seq);
        } else {
            sprintf(hello_str, "-");
        }

        char hack_str[10];
        if( dsp->n_hacks > 0 ) {
            sprintf(hack_str, "%d", dsp->n_hacks);
        } else {
            sprintf(hack_str, "-");
        }

        const char* addr_str = is_unicast_message(msg) ? "UNICAST" : "BROADCAST";

        char str[200];
        sprintf(str, "HELLO=[%s] HACK=[%s] [%s]", hello_str, hack_str, addr_str);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "SEND MESSAGE", str);
        #endif

        if( dsp->hello ) {
            state->stats.total_hellos++;
            free(dsp->hello);
        }

        if( dsp->hacks ) {
            state->stats.total_hacks += dsp->n_hacks;

            free(dsp->hacks);
        }

        free(dsp);
    }

}

DiscoverySendPack* DF_handleDiscoveryEvent(discovery_framework_state* state, DiscoveryInternalEventType event_type, void* event_args, YggMessage* msg) {

    DiscoveryInternalEventResult* der = DA_triggerEvent(state->args->algorithm, event_type, event_args, state->neighbors, msg);

    if( der ) {
        HelloMessage* hello = NULL;
        unsigned char n_hacks = 0;
        HackMessage* hacks = NULL;

        if( der->create_hello ) {
            hello = malloc(sizeof(HelloMessage));
            DF_createHello(state, hello, der->request_replies);
        }

        if( der->create_hack ) {
            if( der->hack_dest ) {
                if(!NE_isPending(der->hack_dest)) {
                    n_hacks = 1;
                    hacks = malloc(sizeof(HackMessage));
                    DF_createHack(state, hacks, der->hack_dest, true);
                } else {
                    n_hacks = 0;
                    hacks = NULL;
                }
            } else {
                DF_createHackBatch(state, &hacks, &n_hacks, state->neighbors);
            }
        }

        DiscoverySendPack* dsp = NULL;

        // If no hello nor hacks don't even call create msg.
        if( hello || hacks ) {
            DF_createMessage(state, hello, hacks, n_hacks, event_type, event_args, msg, (is_memory_zero(&der->dest_addr, WLAN_ADDR_LEN) != 0 ? NULL : &der->dest_addr));

            dsp = newDiscoverySendPack(hello, n_hacks, hacks);
        }

        free(der);
        return dsp;
    } else {
        unsigned short buffer_size = 0;
        pushPayload(msg, (char*)&buffer_size, sizeof(buffer_size), DISCOVERY_FRAMEWORK_PROTO_ID, &msg->destAddr);
        return NULL;
    }

}

void DF_createMessage(discovery_framework_state* state, HelloMessage* hello, HackMessage* hacks, byte n_hacks, DiscoveryInternalEventType event_type, void* event_args, YggMessage* msg, WLANAddr* addr) {
    assert(msg && (hello || hacks));

    // Serialize Message
    byte buffer[YGG_MESSAGE_PAYLOAD];
    unsigned short buffer_size = 0;

    bool send = false;
    if( hello || hacks ) {
        DA_createMessage(state->args->algorithm, state->myID, state->neighbors, event_type, event_args, &state->current_time, hello, hacks, n_hacks, buffer + sizeof(buffer_size), &buffer_size);

        memcpy(buffer, &buffer_size, sizeof(buffer_size));

        // only non periodic nor reply msgs can be canceled
        //send = (msg_type == PERIODIC_MSG || msg_type == REPLY_MSG) ? true : result;
        send = true;
    }

    if(send) {
        WLANAddr* bcast_addr = getBroadcastAddr();
        bool is_unicast = addr != NULL;
        WLANAddr* dest_addr = is_unicast ? addr : bcast_addr;

        pushPayload(msg, (char*)buffer, sizeof(buffer_size) + buffer_size, DISCOVERY_FRAMEWORK_PROTO_ID, dest_addr);

        free(bcast_addr);

        if( hello ) {
            copy_timespec(&state->last_hello_time, &state->current_time);

            // Re-schedule
            scheduleHelloTimer(state, false);

            state->stats.total_hellos++;
        }

        if( hacks ) {
            copy_timespec(&state->last_hack_time, &state->current_time);

            if( !is_unicast ) {
                // Re-schedule
                scheduleHackTimer(state, false);
            }

            state->stats.total_hacks += n_hacks;
        }

        //ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REGISTER TRAFFIC", "");
        DE_registerOutTraffic(state->environment, &state->current_time);
    }

    /*
    else {
        buffer_size = 0;

        pushPayload(msg, (char*)&buffer_size, sizeof(buffer_size), DISCOVERY_FRAMEWORK_PROTO_ID, addr);

        if(hello) {
            // Decrement SEQ
            state->my_seq = dec_seq(state->my_seq, state->args->ignore_zero_seq);
        }
    }
*/

}
