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
    copy_timespec(&state->set_neighbor_change_time, &zero_timespec);
    memset(&state->neighbor_change_summary, 0, sizeof(state->neighbor_change_summary));

    // Stats
	memset(&state->stats, 0, sizeof(discovery_stats));

    // Discovery Environment Timer
    genUUID(state->discovery_environment_timer_id);
    if(state->args->discov_env_refresh_period_s > 0) {
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
        NE_getNeighborType(neigh, &state->current_time));

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
    *n_hacks = NT_getSize(state->neighbors);
    *hacks = malloc(*n_hacks * sizeof(HackMessage));

    if( n_hacks > 0 ) {
        // Compute Next Hack Period
        struct timespec aux;
        subtract_timespec(&aux, &state->current_time, &state->last_hack_time);
        unsigned long elapsed_time_ms = timespec_to_milli(&aux);
        DA_computeNextHackPeriod(state->args->algorithm, elapsed_time_ms, state->args->announce_transition_period_n, state->neighbors, &state->current_time);

        void* iterator = NULL;
        NeighborEntry* current_neigh = NULL;
        HackMessage* current_hack = *hacks;
        while ( (current_neigh = NT_nextNeighbor(state->neighbors, &iterator)) ) {
            DF_createHack(state, current_hack++, current_neigh, false);
        }
    } /*
    else {
        // Re-schedule
        scheduleHackTimer(state, false);
    }
    */
}

bool DF_sendMessage(discovery_framework_state* state, HelloMessage* hello, HackMessage* hacks, byte n_hacks, WLANAddr* addr, MessageType msg_type, void* aux_info) {

    YggMessage msg;
    YggMessage_initBcast(&msg, DISCOVERY_FRAMEWORK_PROTO_ID);

    WLANAddr* bcast_addr = getBroadcastAddr();
    WLANAddr* dest_addr = addr ? addr : bcast_addr;

    bool send = DF_createMessage(state, &msg, hello, hacks, n_hacks, dest_addr, msg_type, aux_info);

    free(bcast_addr);

    if( send ) {
        // Insert into dispatcher queue
        DF_dispatchMessage(state->dispatcher_queue, &msg);

        state->stats.discovery_messages++;
    }

    /*
    #ifdef DEBUG_DISCOVERY
        char str[200];
        sprintf(str, "HELLO=[%d bytes] HACK=[%d bytes]", hello_size, hack_size);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "SENDING MESSAGE", str);
    #endif
    */

    return send;
}

void DF_processMessage(discovery_framework_state* state, byte* data, unsigned short size, bool piggybacked, WLANAddr* mac_addr) {

    bool neighbor_change = DA_processDiscoveryMessage(state->args->algorithm, state, state->myID, &state->current_time, state->neighbors, piggybacked, mac_addr, data, size);

    if(state->dirty_neighborhood) {
        DF_notifyNeighborhood(state);
    }

    if(neighbor_change) {
        scheduleNeighborChange(state, false, false, false, true);
    }
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

        unsigned long max_jitter_ms = lMin(state->args->max_jitter_ms, allowed_max_jitter_ms);

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

        unsigned long max_jitter_ms = lMin(state->args->max_jitter_ms, allowed_max_jitter_ms);

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

void DF_uponHelloTimer(discovery_framework_state* state, bool periodic, bool send_hack) {
    assert( state->hello_timer_active );

    if( DA_periodicHello(state->args->algorithm) ) {

        if( compare_timespec(&state->current_time, &state->next_hello_time) >= 0 ) {
            {
                unsigned long hello_period_ms = DA_getHelloTransmitPeriod(state->args->algorithm, &state->current_time)*1000;
                unsigned long real_period_ms = hello_period_ms - state->args->period_margin_ms;

                struct timespec diff_ = {0};
                subtract_timespec(&diff_, &state->current_time, &state->last_hello_time);
                unsigned long diff = timespec_to_milli(&diff_);

                bool first_timer = compare_timespec(&state->last_hello_time, (struct timespec*)&zero_timespec) == 0;

                assert( first_timer ? true : diff <= real_period_ms );
            }

            state->hello_timer_active = false;
            // copy_timespec(&state->last_hello_time, &state->current_time);

            HelloMessage hello;
            bool request_replies = DA_replyHacksToHellos(state->args->algorithm) != NO_HACK_REPLY;
            DF_createHello(state, &hello, request_replies);

            unsigned char n_hacks = 0;
            HackMessage* hacks = NULL;

            PiggybackType hack_piggyback_type = DA_piggybackHacks(state->args->algorithm);
            bool piggyback_hack = hack_piggyback_type == PIGGYBACK_ON_BROADCAST_TRAFFIC || hack_piggyback_type == PIGGYBACK_ON_DISCOVERY_TRAFFIC || hack_piggyback_type == PIGGYBACK_ON_ALL_TRAFFIC || send_hack;
            if( piggyback_hack ) {
                DF_createHackBatch(state, &hacks, &n_hacks, state->neighbors);
            }

            bool sent = false;
            //  if( periodic ) {
                sent = DF_sendMessage(state, &hello, hacks, n_hacks, NULL, PERIODIC_MSG, NULL);
            /*
            } else {
                void* aux_info = &state->neighbor_change_summary;
                sent = DF_sendMessage(state, &hello, hacks, n_hacks, NULL, NEIGHBOR_CHANGE_MSG, aux_info);
            }
            */

            if( sent ) {
                if(piggyback_hack) {
                    state->stats.piggybacked_hacks += n_hacks;
                }

            }

            // Re-schedule
            // scheduleHelloTimer(state, false);

            if( hacks ) {
                free(hacks);
            }

            #ifdef DEBUG_DISCOVERY
            char aux_str[30];
            if( piggyback_hack )
                sprintf(aux_str, "%d hacks", n_hacks);
            else
                sprintf(aux_str, "NULL");

            char str[200];
            sprintf(str, "HELLO=[SEQ=%hu PERIOD=%d s] HACK=[%s]", hello.seq, hello.period, aux_str);
            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "HELLO TIMER", str);
            #endif

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

void DF_uponHackTimer(discovery_framework_state* state, bool periodic) {
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

            // Create an hack for each neighbor
            byte n_hacks = 0;
            HackMessage* hacks = NULL;
            DF_createHackBatch(state, &hacks, &n_hacks, state->neighbors);

            PiggybackType hello_piggyback_type = DA_piggybackHellos(state->args->algorithm);
            bool piggyback_hello = hello_piggyback_type == PIGGYBACK_ON_BROADCAST_TRAFFIC || hello_piggyback_type == PIGGYBACK_ON_DISCOVERY_TRAFFIC || hello_piggyback_type == PIGGYBACK_ON_ALL_TRAFFIC;

            bool sent = false;

            HelloMessage hello;
            if( periodic ) {
                if( piggyback_hello ) {
                    DF_createHello(state, &hello, false);
                    sent = DF_sendMessage(state, &hello, hacks, n_hacks, NULL, PERIODIC_MSG, NULL);

                    if( sent ) {
                        state->stats.piggybacked_hellos++;
                    }
                } else {
                    sent = DF_sendMessage(state, NULL, hacks, n_hacks, NULL, PERIODIC_MSG, NULL);
                }
            } else {
                assert(!piggyback_hello);
                void* aux_info = &state->neighbor_change_summary;
                sent = DF_sendMessage(state, NULL, hacks, n_hacks, NULL, NEIGHBOR_CHANGE_MSG, aux_info);
            }

            // Re-schedule
            //scheduleHackTimer(state, false);

            #ifdef DEBUG_DISCOVERY
            char aux_str[30];
            if( piggyback_hello )
                sprintf(aux_str, "SEQ=%hu PERIOD=%d s", hello.seq, hello.period);
            else
                sprintf(aux_str, "NULL");

            char str[200];
            sprintf(str, "HELLO=[%s] HACK=[%d hacks]", aux_str, n_hacks);
            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "HACK TIMER", str);
            #endif

            if( hacks ) {
                free(hacks);
            }

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
    assert( DA_replyHacksToHellos(state->args->algorithm) != NO_HACK_REPLY );

    unsigned char* neigh_id = timer_payload;
    NeighborEntry* neigh = NT_getNeighbor(state->neighbors, neigh_id);
    assert(neigh);

    unsigned char n_hacks = 1;
    HackMessage hack;
    DF_createHack(state, &hack, neigh, true);

    WLANAddr* addr = NULL;
    HackReplyType reply_type = DA_replyHacksToHellos(state->args->algorithm);
    if( reply_type == UNICAST_HACK_REPLY ) {
        addr = NE_getNeighborMAC(neigh);
    }

    PiggybackType hello_piggyback_type = DA_piggybackHellos(state->args->algorithm);
    bool piggyback_hello = hello_piggyback_type == PIGGYBACK_ON_BROADCAST_TRAFFIC || hello_piggyback_type == PIGGYBACK_ON_DISCOVERY_TRAFFIC || hello_piggyback_type == PIGGYBACK_ON_ALL_TRAFFIC;

    HelloMessage hello;
    if( piggyback_hello ) {
        DF_createHello(state, &hello, false); // request_replies is set to false

        bool sent = DF_sendMessage(state, &hello, &hack, n_hacks, addr, REPLY_MSG, NULL);
        if(sent) {
            state->stats.piggybacked_hellos++;
        }
    } else {
        DF_sendMessage(state, NULL, &hack, n_hacks, addr, REPLY_MSG, NULL);
    }

    #ifdef DEBUG_DISCOVERY
    char hello_str[30];
    if( piggyback_hello ) {
        sprintf(hello_str, "SEQ=%hu PERIOD=%d s", hello.seq, hello.period);
    } else {
        sprintf(hello_str, "NULL");
    }

    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(hack.dest_process_id, id_str);

    char str[200];
    sprintf(str, "HELLO=[%s] HACK=[to %s SEQ=%hu]", hello_str, id_str, hack.seq);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REPLY TIMER", str);
    #endif
}

bool DF_uponNeighborTimer(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    // Last Timer
    struct timespec last_neighbor_timer;
    copy_timespec(&last_neighbor_timer, NE_getLastNeighborTimer(neigh));
    NE_setLastNeighborTimer(neigh, &state->current_time);

    struct timespec* rx_exp_time = NE_getNeighborRxExpTime(neigh);
    struct timespec* tx_exp_time = NE_getNeighborTxExpTime(neigh);

    unsigned long next_timer = 0;

    NeighborTimerSummary* summary = newNeighborTimerSummary();

    // Check if rx not expired
    if( compare_timespec(rx_exp_time, &state->current_time) > 0 ) {

        // Compute missed hellos
        unsigned long hello_period = NE_getNeighborHelloPeriod(neigh)*1000;
        unsigned int hello_misses = state->args->hello_misses;

        unsigned int missed_hellos = compute_missed(hello_misses, hello_period, rx_exp_time, &state->current_time);

        unsigned int prev_missed_hellos = compute_missed(hello_misses, hello_period, rx_exp_time, &last_neighbor_timer);

        if( missed_hellos > prev_missed_hellos ) {
            unsigned int lost = 1;
            summary->missed_hellos++;

            state->stats.missed_hellos++;

            // Update Link Quality
            double old_rx_lq = NE_getRxLinkQuality(neigh);
            double new_rx_lq = DA_computeLinkQuality(state->args->algorithm, NE_getLinkQualityAttributes(neigh), old_rx_lq, 0, lost, false, &state->current_time);

            double lq_delta = fabs(old_rx_lq - new_rx_lq);
            if( lq_delta >= state->args->lq_epsilon || (lq_delta > 0 && (new_rx_lq == 1.0 || new_rx_lq == 0.0)) ) {
                NE_setRxLinkQuality(neigh, new_rx_lq);
                summary->updated_quality = true;

                if( lq_delta >= state->args->lq_threshold ) {
                    summary->updated_quality_threshold = true;
                }
            }
        }

        unsigned long next_hello_miss = compute_next_moment(rx_exp_time, &state->current_time, hello_misses, missed_hellos, hello_period);

        // Check if tx not expired
        if( compare_timespec(tx_exp_time, &state->current_time) > 0 ) {
            unsigned long hack_period = NE_getNeighborHackPeriod(neigh)*1000;
            unsigned int hack_misses = state->args->hack_misses;

            unsigned int missed_hacks = compute_missed(hack_misses, hack_period, tx_exp_time, &state->current_time);

            unsigned int prev_missed_hacks = compute_missed(hack_misses, hack_period, tx_exp_time, &last_neighbor_timer);

            if( missed_hacks > prev_missed_hacks ) {
                state->stats.missed_hacks++;

                summary->missed_hacks++;
            }

            unsigned long next_hack_miss = compute_next_moment(tx_exp_time, &state->current_time, hack_misses, missed_hacks, hack_period);

            // Log
            #ifdef DEBUG_DISCOVERY
            if( missed_hellos > 0 || missed_hacks > 0 ) {
                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(NE_getNeighborID(neigh), id_str);

                char str[200];
                sprintf(str, "%s    missed hellos: %u/%u    missed hacks: %u/%u", id_str, missed_hellos, hello_misses, missed_hacks, hack_misses);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBOR TIMER", str);
            }
            #endif

            next_timer = lMin(next_hello_miss, next_hack_miss);
        } else {

            // Check if tx expired after the previous neighbor timer
            if( compare_timespec(tx_exp_time, &last_neighbor_timer) > 0 ) {
                summary->lost_bi = true;

                // Log
                #ifdef DEBUG_DISCOVERY
                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(NE_getNeighborID(neigh), id_str);

                char str[200];
                sprintf(str, "%s", id_str);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "LOST BI", str);
                #endif
            }

            // Log
            #ifdef DEBUG_DISCOVERY
            if( missed_hellos > 0 ) {
                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(NE_getNeighborID(neigh), id_str);

                char str[200];
                sprintf(str, "%s    missed hellos: %u/%u    missed hacks: expired", id_str, missed_hellos, hello_misses);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBOR TIMER", str);
            }
            #endif

            next_timer = next_hello_miss;
        }

        // 2-hop Neighs GC + compute next expiration
        struct timespec min_exp;
        bool first = true;
        hash_table* ht = NE_getTwoHopNeighbors(neigh);
        void* iterator = NULL;
        hash_table_item* hit = NULL;
        while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
            TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;
            if( compare_timespec(THNE_getExpiration(nn), &state->current_time) < 0 ) {
                hash_table_remove_item(ht, THNE_getID(nn));
                free(nn);
                free(hit);
                summary->deleted_2hop++;
            } else {
                if( first || compare_timespec(THNE_getExpiration(nn), &min_exp) < 0 ) {
                    first = false;
                    copy_timespec(&min_exp, THNE_getExpiration(nn));
                }
            }
        }
        summary->lost_2hop_neighbor = summary->deleted_2hop > 0;

        /*if( deleted_2hop > 0 ) {
            // printf("lost 2-hop neigh(s)\n");
            scheduleNeighborChange(state, 2);
        }*/

        subtract_timespec(&min_exp, &min_exp, &state->current_time);
        unsigned long nn_exp =  timespec_to_milli(&min_exp);

        next_timer = lMin(next_timer, nn_exp);
    } else {
        // Neighbor is dead
        if( !NE_isDeleted(neigh) ) {
            NE_setDeleted(neigh, &state->current_time);

            state->stats.lost_neighbors++;

            summary->lost_neighbor = true;

            struct timespec removal_time;
            milli_to_timespec(&removal_time, state->args->neigh_hold_time_s*1000);
            add_timespec(&removal_time, &removal_time,  &state->current_time);
            NE_setNeighborRemovalTime(neigh, &removal_time);

            next_timer = state->args->neigh_hold_time_s*1000;
        } else {
            if( compare_timespec(NE_getNeighborRemovalTime(neigh), &state->current_time) <= 0 ) {
                #ifdef DEBUG_DISCOVERY
                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(NE_getNeighborID(neigh), id_str);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REMOVED NEIGHBOR", id_str);
                #endif

                flushNeighbor(state, neigh);
                neigh = NULL;

                summary->removed = true;
            }
        }
    }

    summary->updated_neighbor = summary->lost_bi || summary->updated_quality;

    if( !summary->removed ) {

        if( summary->lost_neighbor ) {
            DF_notifyLostNeighbor(state, neigh);

            scheduleNeighborChange(state, NULL, NULL, summary, false);

            DE_registerLostNeighbor(state->environment, &state->current_time);
        } else {
            if( summary->updated_neighbor || summary->lost_2hop_neighbor ) {
                DF_notifyUpdateNeighbor(state, neigh);

                if( summary->lost_bi || summary->updated_quality_threshold || summary->lost_2hop_neighbor ) {
                    scheduleNeighborChange(state, NULL, NULL, summary, false);
                }
            }
        }

        struct timespec t;
        milli_to_timespec(&t, next_timer);
        SetTimer(&t, NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_TIMER);
    }

    bool removed = summary->removed;

    free(summary);

    return removed;
}

void DF_uponDiscoveryEnvironmentTimer(discovery_framework_state* state) {

    bool changed = false;

    unsigned int n_neighbors = 0;
    double neighbors_density = 0.0;
    double in_traffic = 0.0;
    void* iterator = NULL;
    for(NeighborEntry* current_neigh = NT_nextNeighbor(state->neighbors, &iterator); current_neigh; current_neigh = NT_nextNeighbor(state->neighbors, &iterator)) {
        if( NE_getNeighborType(current_neigh, &state->current_time) != LOST_NEIGH ) {
            in_traffic += NE_getOutTraffic(current_neigh);
            n_neighbors++;
            neighbors_density += NE_getTwoHopNeighbors(current_neigh)->n_items;
        }
    }

    if( n_neighbors > 0 ) {
        neighbors_density += n_neighbors;
        neighbors_density /= (n_neighbors+1);
    }

    changed |= DE_setInTraffic(state->environment, in_traffic, state->args->traffic_epsilon);

    changed |= DE_computeOutTraffic(state->environment, &state->current_time, state->args->traffic_window_type, state->args->traffic_epsilon);

    changed |= DE_computeNewNeighborsFlux(state->environment, &state->current_time, state->args->churn_window_type, state->args->churn_epsilon);

    changed |= DE_computeLostNeighborsFlux(state->environment, &state->current_time, state->args->churn_window_type, state->args->churn_epsilon);

    changed |= DE_setNNeighbors(state->environment, n_neighbors);

    changed |= DE_setNeigbhorsDensity(state->environment, neighbors_density, state->args->neigh_density_epsilon);

    if( changed ) {
        // Log
        #ifdef DEBUG_DISCOVERY
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

    struct timespec* rx_exp_time = NE_getNeighborRxExpTime(neigh);
    struct timespec* tx_exp_time = NE_getNeighborTxExpTime(neigh);

    unsigned long next_timer = 0;

    // Check if rx not expired
    if( compare_timespec(rx_exp_time, &state->current_time) > 0 ) {

            // Compute missed hellos
            unsigned long hello_period = NE_getNeighborHelloPeriod(neigh)*1000;
            unsigned int hello_misses = state->args->hello_misses;

            unsigned int missed_hellos = compute_missed(hello_misses, hello_period, rx_exp_time, &state->current_time);

            unsigned long next_hello_miss =  compute_next_moment(rx_exp_time, &state->current_time, hello_misses, missed_hellos, hello_period);

            // Check if tx not expired
            if( compare_timespec(tx_exp_time, &state->current_time) > 0 ) {
                unsigned long hack_period = NE_getNeighborHackPeriod(neigh)*1000;
                unsigned int hack_misses = state->args->hack_misses;

                unsigned int missed_hacks = compute_missed(hack_misses, hack_period, tx_exp_time, &state->current_time);

                unsigned long next_hack_miss = compute_next_moment(tx_exp_time, &state->current_time, hack_misses, missed_hacks, hack_period);

                next_timer = lMin(next_hello_miss, next_hack_miss);
            } else {
                next_timer = next_hello_miss;
            }

            // 2-hop Neighs GC + compute next expiration
            struct timespec min_exp;
            bool first = true;
            hash_table* ht = NE_getTwoHopNeighbors(neigh);
            void* iterator = NULL;
            hash_table_item* hit = NULL;
            while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
                TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;
                if( compare_timespec(THNE_getExpiration(nn), &state->current_time) < 0 ) {
                    first = false;
                    next_timer = 0;
                    break;
                } else {
                    if( first || compare_timespec(THNE_getExpiration(nn), &min_exp) < 0 ) {
                        first = false;
                        copy_timespec(&min_exp, THNE_getExpiration(nn));
                    }
                }
            }

            subtract_timespec(&min_exp, &min_exp, &state->current_time);
            unsigned long nn_exp =  timespec_to_milli(&min_exp);

            next_timer = lMin(next_timer, nn_exp);
        } else {
            // Neighbor is dead
            if( !NE_isDeleted(neigh) ) {
                next_timer = 0;
            } else {
                if( compare_timespec(NE_getNeighborRemovalTime(neigh), &state->current_time) <= 0 ) {
                    next_timer = 0;
                } else {
                    struct timespec aux;
                    subtract_timespec(&aux, NE_getNeighborRemovalTime(neigh), &state->current_time);
                    next_timer = timespec_to_milli(&aux);
                }
            }
        }

        // TODO: compare with old timer
        CancelTimer(NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID);
        struct timespec t;
        milli_to_timespec(&t, next_timer);
        SetTimer(&t, NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_TIMER);
}

void flushNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    NT_removeNeighbor(state->neighbors, NE_getNeighborID(neigh));

    void* lq_attrs = NULL, *msg_attrs = NULL;
    destroyNeighborEntry(neigh, &lq_attrs, &msg_attrs);

    DA_destroyLinkQualityAttributes(state->args->algorithm, lq_attrs);

    DA_destroyMessageAttributes(state->args->algorithm, msg_attrs);
}

void scheduleReply(discovery_framework_state* state, HelloMessage* hello) {

    if( DA_replyHacksToHellos(state->args->algorithm) != NO_HACK_REPLY  && hello->request_replies ) {

            unsigned long jitter = (unsigned long)(randomProb()*state->args->max_jitter_ms);

            struct timespec t_ = {0};
            milli_to_timespec(&t_, jitter);

            SetTimerWithPayload(&t_, NULL, DISCOVERY_FRAMEWORK_PROTO_ID, REPLY_TIMER, hello->process_id, sizeof(uuid_t));
    }

}

void DF_uponNeighborChangeTimer(discovery_framework_state* state) {
    assert( state->neighbor_change_timer_active );

    NeighborChangeSummary* summary = &state->neighbor_change_summary;

    DiscoveryAlgorithm* alg = state->args->algorithm;

    bool other = summary->other;

    bool new_neighbor = summary->new_neighbor;
    bool lost_neighbor = summary->lost_neighbor;
    bool updated_neighbor = summary->updated_neighbor;

    bool new_2hop_neighbor = summary->added_2hop_neighbor;
    bool lost_2hop_neighbor = summary->lost_2hop_neighbor;
    bool updated_2hop_neighbor = summary->updated_2hop_neighbor;

    bool send_hello = other || \
    (DA_HelloNewNeighbor(alg) && new_neighbor) || \
    (DA_HelloLostNeighbor(alg)  && lost_neighbor) || \
    (DA_HelloUpdateNeighbor(alg) && updated_neighbor) || \
    (DA_HelloNew2HopNeighbor(alg) && new_2hop_neighbor) || \
    (DA_HelloLost2HopNeighbor(alg)  && lost_2hop_neighbor) || \
    (DA_HelloUpdate2HopNeighbor(alg) && updated_2hop_neighbor);

    bool send_hack = other || \
    (DA_HackNewNeighbor(alg) && new_neighbor) || \
    (DA_HackLostNeighbor(alg)  && lost_neighbor) || \
    (DA_HackUpdateNeighbor(alg) && updated_neighbor) || \
    (DA_HackNew2HopNeighbor(alg) && new_2hop_neighbor) || \
    (DA_HackLost2HopNeighbor(alg)  && lost_2hop_neighbor) || \
    (DA_HackUpdate2HopNeighbor(alg) && updated_2hop_neighbor);

    YggMessage msg;
    YggMessage_initBcast(&msg, DISCOVERY_FRAMEWORK_PROTO_ID);

    WLANAddr* bcast_addr = getBroadcastAddr();

    HelloMessage hello_;
    HelloMessage* hello = &hello_;
    if( send_hello ) {
        DF_createHello(state, hello, false);
    }

    unsigned char n_hacks = 0;
    HackMessage* hacks = NULL;

    if( send_hack ) {
        DF_createHackBatch(state, &hacks, &n_hacks, state->neighbors);
    }

    void* aux_info = &state->neighbor_change_summary;
    bool send = DF_createMessage(state, &msg, hello, hacks, n_hacks, bcast_addr, NEIGHBOR_CHANGE_MSG, aux_info);

    free(bcast_addr);

    if( hacks ) {
        free(hacks);
    }

    if( send ) {
        // Insert into dispatcher queue
        DF_dispatchMessage(state->dispatcher_queue, &msg);

        state->stats.discovery_messages++;
    }

    state->neighbor_change_timer_active = false;
    memset(&state->neighbor_change_summary, 0, sizeof(state->neighbor_change_summary));

    // Log
    #ifdef DEBUG_DISCOVERY
    char str[200];
    sprintf(str, "HELLO=[%s] HACK=[%s]", (send_hello ? "yes" : "no"), (send_hack ? "yes" : "no"));
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBOR-CHANGE TIMER", str);
    #endif
}

void scheduleNeighborChange(discovery_framework_state* state, HelloDeliverSummary* hello_summary, HackDeliverSummary* hack_summary, NeighborTimerSummary* neighbor_timer_summary, bool other) {

    NeighborChangeSummary summary;
    memset(&summary, 0, sizeof(summary));

    if( hello_summary ) {
        summary.new_neighbor = hello_summary->new_neighbor;
        summary.updated_neighbor = hello_summary->updated_neighbor;
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
        summary.other = other;
    //}

    DiscoveryAlgorithm* alg = state->args->algorithm;

    // New schedule
    if( !state->neighbor_change_timer_active ) {
        bool other = summary.other;

        bool new_neighbor = summary.new_neighbor;
        bool lost_neighbor = summary.lost_neighbor;
        bool updated_neighbor = summary.updated_neighbor;

        bool new_2hop_neighbor = summary.added_2hop_neighbor;
        bool lost_2hop_neighbor = summary.lost_2hop_neighbor;
        bool updated_2hop_neighbor = summary.updated_2hop_neighbor;

        bool send_hello = other || \
        (DA_HelloNewNeighbor(alg) && new_neighbor) || \
        (DA_HelloLostNeighbor(alg)  && lost_neighbor) || \
        (DA_HelloUpdateNeighbor(alg) && updated_neighbor) || \
        (DA_HelloNew2HopNeighbor(alg) && new_2hop_neighbor) || \
        (DA_HelloLost2HopNeighbor(alg)  && lost_2hop_neighbor) || \
        (DA_HelloUpdate2HopNeighbor(alg) && updated_2hop_neighbor);

        bool send_hack = other || \
        (DA_HackNewNeighbor(alg) && new_neighbor) || \
        (DA_HackLostNeighbor(alg)  && lost_neighbor) || \
        (DA_HackUpdateNeighbor(alg) && updated_neighbor) || \
        (DA_HackNew2HopNeighbor(alg) && new_2hop_neighbor) || \
        (DA_HackLost2HopNeighbor(alg)  && lost_2hop_neighbor) || \
        (DA_HackUpdate2HopNeighbor(alg) && updated_2hop_neighbor);

        if( send_hello || send_hack ) {

            state->neighbor_change_timer_active = true;
            copy_timespec(&state->set_neighbor_change_time, &state->current_time);

            memcpy(&state->neighbor_change_summary, &summary, sizeof(NeighborChangeSummary));

            // Compute jitter
            unsigned long max_jitter = state->args->max_jitter_ms;

            struct timespec aux;
            subtract_timespec(&aux, &state->next_hello_time, &state->current_time);
            unsigned long aux2 = timespec_to_milli(&aux);
            if( aux2 < state->args->max_jitter_ms ) {
                max_jitter = aux2;
            }

            subtract_timespec(&aux, &state->next_hack_time, &state->current_time);
            aux2 = timespec_to_milli(&aux);
            if( aux2 < state->args->max_jitter_ms ) {
                if( aux2 < max_jitter ) {
                    max_jitter = aux2;
                }
            }

            struct timespec t_ = {0};

            if( max_jitter < state->args->max_jitter_ms ) {
                milli_to_timespec(&t_, max_jitter > 1 ? max_jitter-1: 0);
            } else {
                unsigned long jitter = (unsigned long)(randomProb()*max_jitter);
                milli_to_timespec(&t_, jitter);
            }

            SetTimer(&t_, state->neighbor_change_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_CHANGE_TIMER);
        }
    }

    // Update Neighbor Change Summary
    else {
        NeighborChangeSummary* s = &state->neighbor_change_summary;

        s->new_neighbor |= summary.new_neighbor;
        s->updated_neighbor |= summary.updated_neighbor;
        s->lost_neighbor |= summary.lost_neighbor;
        s->updated_2hop_neighbor |= summary.updated_2hop_neighbor;
        s->added_2hop_neighbor |= summary.added_2hop_neighbor;
        s->lost_2hop_neighbor |= summary.lost_2hop_neighbor;
        s->other |= summary.other;
        s->removed |= summary.removed;
        s->rebooted |= summary.rebooted;
        s->lost_bi |= summary.lost_bi;
        s->became_bi |= summary.became_bi;
        s->hello_period_changed |= summary.hello_period_changed;
        s->hack_period_changed |= summary.hack_period_changed;
        s->updated_quality |= summary.updated_quality;
        s->updated_quality_threshold |= summary.updated_quality_threshold;
    }

}

HelloDeliverSummary* deliverHello(void* f_state, HelloMessage* hello, WLANAddr* addr) {
    return DF_uponHelloMessage((discovery_framework_state*)f_state, hello, addr);
}

HackDeliverSummary* deliverHack(void* f_state, HackMessage* hack) {
    return DF_uponHackMessage((discovery_framework_state*)f_state, hack);
}

HelloDeliverSummary* newHelloDeliverSummary() {
    HelloDeliverSummary* summary = malloc(sizeof(HelloDeliverSummary));

    summary->new_neighbor = false;
    summary->updated_neighbor = false;
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
    #ifdef DEBUG_DISCOVERY
    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(hello->process_id, id_str);

    char str[200];
    sprintf(str, "received an HELLO from %s with SEQ=%hu and PERIOD=%d s", id_str, hello->seq, hello->period);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "RCV HELLO", str);
    #endif

    HelloDeliverSummary* summary = newHelloDeliverSummary();

    NeighborEntry* neigh = NT_getNeighbor(state->neighbors, hello->process_id);

    struct timespec rx_exp_time;
    milli_to_timespec(&rx_exp_time, hello->period*1000*state->args->hello_misses);
    add_timespec(&rx_exp_time, &rx_exp_time, &state->current_time);

    if( neigh ) {
        int seq_cmp = compare_seq(hello->seq, NE_getNeighborSEQ(neigh), state->args->ignore_zero_seq);
        summary->rebooted = seq_cmp < 0;

        if( NE_isDeleted(neigh) || summary->rebooted ) {
            flushNeighbor(state, neigh);
            neigh = NULL;

            CancelTimer(hello->process_id, DISCOVERY_FRAMEWORK_PROTO_ID);

            if(summary->rebooted) {
                // Log
                #ifdef DEBUG_DISCOVERY
                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(hello->process_id, id_str);

                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REBOOTED", id_str);
                #endif
            }
        }
    }

    if( neigh ) {
        unsigned int prev_missed_hellos = compute_missed(state->args->hello_misses, NE_getNeighborHelloPeriod(neigh)*1000, NE_getNeighborRxExpTime(neigh), NE_getLastNeighborTimer(neigh));

        int seq_cmp = compare_seq(hello->seq, NE_getNeighborSEQ(neigh), state->args->ignore_zero_seq);
        assert(seq_cmp >= 0);

        int missed_hellos = 0;
        if( seq_cmp > 0 ) {
            if( prev_missed_hellos <= seq_cmp - 1 ) {
                missed_hellos = seq_cmp - 1 - prev_missed_hellos;
            } else {
                missed_hellos = 0;

                // Log
                // #ifdef DEBUG_DISCOVERY
                char str[200];
                sprintf(str, "prev_missed_hellos=%u   seq_cmp-1=%d", missed_hellos, seq_cmp-1);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "HELLO SEQ ERROR", str);
                // #endif
            }
        } else {
            missed_hellos = 0;
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

        NE_setMessageAttributes(neigh, DA_createMessageAttributes(state->args->algorithm));

        NT_addNeighbor(state->neighbors, neigh);

        summary->new_neighbor = summary->rebooted ? false : true;
        summary->missed_hellos = 0;

        struct timespec t;
        milli_to_timespec(&t, hello->period*1000);
        SetTimer(&t, hello->process_id, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_TIMER);

        if(!summary->rebooted)
            state->stats.new_neighbors++;
    }

    // Update Link Quality
    double old_rx_lq = NE_getRxLinkQuality(neigh);
    double new_rx_lq = DA_computeLinkQuality(state->args->algorithm, NE_getLinkQualityAttributes(neigh), old_rx_lq, 1, summary->missed_hellos, summary->new_neighbor, &state->current_time);

    double lq_delta = fabs(old_rx_lq - new_rx_lq);
    if( lq_delta >= state->args->lq_epsilon || (lq_delta > 0 && (new_rx_lq == 1.0 || new_rx_lq == 0.0)) ) {
        NE_setRxLinkQuality(neigh, new_rx_lq);
        summary->updated_quality = true;

        if( lq_delta >= state->args->lq_threshold ) {
            summary->updated_quality_threshold = true;
        }
    }

    // Update traffic
    double traffic_delta = fabs(NE_getOutTraffic(neigh) - hello->traffic);
    if( traffic_delta >= state->args->traffic_epsilon || (traffic_delta > 0 && hello->traffic == 0.0) ) {
        NE_setOutTraffic(neigh, hello->traffic);
        summary->updated_traffic = true;

        if( traffic_delta >= state->args->traffic_threshold ) {
            summary->updated_traffic_threshold = true;
        }
    }

    // Re-schedule neighbor timer
    if( summary->period_changed ) {
        scheduleNeighborTimer(state, neigh);
    }

    // Notify
    if( summary->new_neighbor ) {
        DF_notifyNewNeighbor(state, neigh);

        scheduleNeighborChange(state, summary, NULL, NULL, false);

        DE_registerNewNeighbor(state->environment, &state->current_time);
    } else if( summary->updated_quality || summary->updated_traffic || summary->rebooted ) {
        summary->updated_neighbor = true;
        DF_notifyUpdateNeighbor(state, neigh);

        if( summary->updated_quality_threshold || summary->updated_traffic_threshold || summary->rebooted ) {
            scheduleNeighborChange(state, summary, NULL, NULL, false);

        }
    }

    scheduleReply(state, hello);

    return summary;
}

HackDeliverSummary* DF_uponHackMessage(discovery_framework_state* state, HackMessage* hack) {

    // Log
    #ifdef DEBUG_DISCOVERY
    char id_str1[UUID_STR_LEN+1];
    id_str1[UUID_STR_LEN] = '\0';
    uuid_unparse(hack->src_process_id, id_str1);

    char id_str2[UUID_STR_LEN+1];
    id_str2[UUID_STR_LEN] = '\0';
    uuid_unparse(hack->dest_process_id, id_str2);

    char str[200];
    sprintf(str, "from %s to %s with SEQ=%hu and PERIOD=%d s", id_str1, id_str2, hack->seq, hack->period);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "RCV HACK", str);
    #endif

    HackDeliverSummary* summary = newHackDeliverSummary();

    NeighborEntry* neigh = NT_getNeighbor(state->neighbors, hack->src_process_id);

    // The neighbor is known already and is not dead
    if( neigh && !NE_isDeleted(neigh) ) {

            // Compute the expiration time for this hack
            struct timespec hack_exp_time;
            milli_to_timespec(&hack_exp_time, hack->period*1000*state->args->hack_misses);
            add_timespec(&hack_exp_time, &hack_exp_time, &state->current_time);

            bool positive_hack = hack->neigh_type != LOST_NEIGH;
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
                        unsigned short expected_seq = NE_getNeighborHSEQ(neigh);
                        int seq_cmp = compare_seq(hack->seq, expected_seq, state->args->ignore_zero_seq);
                        int seq_cmp2 = compare_seq(hack->seq, dec_seq(state->my_seq, state->args->ignore_zero_seq), state->args->ignore_zero_seq);

                        summary->new_hack = seq_cmp > 0;
                        summary->repeated_yet_fresh_hack = seq_cmp == 0 && seq_cmp2 >= 0;

                        // Compute previously missed hacks, from the reception of the last valid hack until the last neighbor timer
                        prev_missed_hacks = compute_missed(state->args->hack_misses, NE_getNeighborHackPeriod(neigh)*1000, NE_getNeighborTxExpTime(neigh), NE_getLastNeighborTimer(neigh));

                        // If there is a difference between the hack's seq and the expected seq, then hack misses occurred
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
                                // #ifdef DEBUG_DISCOVERY
                                char str[200];
                                sprintf(str, "prev_missed_hacks=%u   seq_cmp-1=%d", missed_hacks, seq_cmp-1);
                                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "HACK SEQ ERROR", str);
                                // #endif
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
                            #ifdef DEBUG_DISCOVERY
                            char id_str[UUID_STR_LEN+1];
                            id_str[UUID_STR_LEN] = '\0';
                            uuid_unparse(hack->src_process_id, id_str);

                            char str[200];
                            sprintf(str, "%s", id_str);
                            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "BECAME BI", str);
                            #endif
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
                    summary->repeated_yet_fresh_hack = seq_cmp == 0 && seq_cmp2 >= 0;

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
                            summary->updated_2hop_traffic = true;

                            if( traffic_delta >= state->args->traffic_threshold ) {
                                summary->updated_2hop_traffic_threshold = true;
                            }
                        }

                        THNE_setExpiration(nn, &hack_exp_time);
                    } else {
                        // stale hack, ignore
                    }

                } else {
                    // Insert new 2-hop neighbor
                    TwoHopNeighborEntry* nn = newTwoHopNeighborEntry(hack->dest_process_id, hack->seq, hack->neigh_type == BI_NEIGH, hack->rx_lq, hack->tx_lq, hack->traffic, &hack_exp_time);

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
                        #ifdef DEBUG_DISCOVERY
                        char id_str[UUID_STR_LEN+1];
                        id_str[UUID_STR_LEN] = '\0';
                        uuid_unparse(hack->src_process_id, id_str);

                        char str[200];
                        sprintf(str, "%s", id_str);
                        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "LOST BI", str);
                        #endif
                    }
                }

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
                        // Remove from 2-hop neighborhood, if present
                        TwoHopNeighborEntry* removed = NE_removeTwoHopNeighborEntry(neigh, hack->dest_process_id);
                        if(removed) {
                            free(removed);
                            summary->lost_2hop_neighbor = true;
                        }
                    }
                }
            }
    } else {
        // The neighbor is not know. Ignore the HACK
    }

    summary->updated_neighbor = summary->became_bi || summary->lost_bi || summary->updated_quality;
    summary->updated_2hop_neighbor = summary->became_bi_2hop || summary->lost_bi_2hop || summary->updated_2hop_quality || summary->updated_2hop_traffic;

    if( summary->updated_neighbor || summary->updated_2hop_neighbor || summary->added_2hop_neighbor || summary->lost_2hop_neighbor ) {
        DF_notifyUpdateNeighbor(state, neigh);

        if( summary->became_bi || summary->lost_bi || summary->updated_quality_threshold || (summary->became_bi_2hop || summary->lost_bi_2hop || summary->updated_2hop_quality_threshold || summary->updated_2hop_traffic_threshold) || summary->added_2hop_neighbor || summary->lost_2hop_neighbor ) {
            scheduleNeighborChange(state, NULL, summary, NULL, false);
        }
    }

    // Re-schedule neighbor timer
    scheduleNeighborTimer(state, neigh);

    return summary;
}

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

void DF_piggybackDiscovery(discovery_framework_state* state, YggMessage* msg) {

    WLANAddr* bcast_addr = getBroadcastAddr();
    WLANAddr* addr = &msg->destAddr;
    bool is_unicast_addr = memcmp(addr->data, bcast_addr->data, WLAN_ADDR_LEN) == 0;
    WLANAddr* dest_addr = is_unicast_addr ? addr : bcast_addr;

    // Piggyback Hello
    PiggybackType hello_piggyback_type = DA_piggybackHellos(state->args->algorithm);
    bool piggyback_hello = !is_unicast_addr && (hello_piggyback_type == PIGGYBACK_ON_BROADCAST_TRAFFIC || hello_piggyback_type == PIGGYBACK_ON_DISCOVERY_TRAFFIC || hello_piggyback_type == PIGGYBACK_ON_ALL_TRAFFIC);

    HelloMessage hello_;
    HelloMessage* hello = &hello_;
    if( piggyback_hello ) {
        DF_createHello(state, hello, false);
    } else {
        hello = NULL;
    }

    // Piggyback Hacks
    PiggybackType hack_piggyback_type = DA_piggybackHacks(state->args->algorithm);
    bool piggyback_hack = (!is_unicast_addr && (hack_piggyback_type == PIGGYBACK_ON_BROADCAST_TRAFFIC || hack_piggyback_type == PIGGYBACK_ON_DISCOVERY_TRAFFIC || hack_piggyback_type == PIGGYBACK_ON_ALL_TRAFFIC)) || (hack_piggyback_type == PIGGYBACK_ON_UNICAST_TRAFFIC && is_unicast_addr);
    unsigned char n_hacks = 0;
    HackMessage* hacks = NULL;
    if( piggyback_hack ) {

        bool unicast_hack = hack_piggyback_type == PIGGYBACK_ON_UNICAST_TRAFFIC;
        if( unicast_hack ) {
            // Find Neighbor with the corresponding MAC addr
            void* iterator = NULL;
            NeighborEntry* current_neigh = NULL;
            NeighborEntry* found_neigh = NULL;
            while ( (current_neigh = NT_nextNeighbor(state->neighbors, &iterator)) ) {
                if( memcmp(NE_getNeighborMAC(current_neigh)->data, addr->data, WLAN_ADDR_LEN) == 0 ) {
                    found_neigh = current_neigh;
                    break;
                }
            }

            if( found_neigh != NULL ) {
                n_hacks = 1;
                hacks = malloc(n_hacks * sizeof(HackMessage));

                DF_createHack(state, hacks, current_neigh, true);
            } else {
                n_hacks = 0;
                hacks = NULL;
            }
        }

        else {
            DF_createHackBatch(state, &hacks, &n_hacks, state->neighbors);
        }
    }

    // TODO: try to leverage promiscuous mode in the future

    bool send = DF_createMessage(state, msg, hello, hacks, n_hacks, dest_addr, PIGGYBACK_MSG, msg);

    if( send ) {

        if( piggyback_hello ) {
            state->stats.piggybacked_hellos++;
        }

        if( piggyback_hack ) {
            state->stats.piggybacked_hacks += n_hacks;
        }
    }

    if( hacks ) {
        free(hacks);
    }

    free(bcast_addr);
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

/*
void DF_printNeighbors(discovery_framework_state* state) {
    char* str1 = NULL, *str2 = NULL;

    DF_uponDiscoveryEnvironmentTimer(state); // temp
    NE_print(state->environment, &str1);

    NT_print(state->neighbors, &str2, &state->current_time, state->myID, &state->myAddr, state->my_seq);

    char str3[strlen(str1)+strlen(str2)+1];
    sprintf(str3, "\n%s\n%s", str1, str2);

    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBORS TABLE", str3);

    free(str1);
    free(str2);
}
*/



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

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, NEW_NEIGHBOR);

    // Append Neighbor ID
    YggEvent_addPayload(ev, NE_getNeighborID(neigh), sizeof(uuid_t));

    // Append MAC addr
    YggEvent_addPayload(ev, NE_getNeighborMAC(neigh), WLAN_ADDR_LEN);

    // Append LQs
    double rx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(ev, &rx_lq, sizeof(double));
    double tx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(ev, &tx_lq, sizeof(double));

    // Append Traffic
    double traffic = NE_getOutTraffic(neigh);
    YggEvent_addPayload(ev, &traffic, sizeof(double));

    // Append Neighbor Type
    byte is_bi = NE_getNeighborType(neigh, &state->current_time) == BI_NEIGH;
    YggEvent_addPayload(ev, &is_bi, sizeof(byte));

    // Append Neighbors
    hash_table* ht = NE_getTwoHopNeighbors(neigh);
    byte n = ht->n_items;
    YggEvent_addPayload(ev, &n, sizeof(byte));

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;

        // Append Neigh ID
        YggEvent_addPayload(ev, THNE_getID(nn), sizeof(uuid_t));

        // Append Neigh LQ
        rx_lq = THNE_getRxLinkQuality(nn);
        YggEvent_addPayload(ev, &rx_lq, sizeof(double));
        tx_lq = THNE_getTxLinkQuality(nn);
        YggEvent_addPayload(ev, &tx_lq, sizeof(double));

        // Append Traffic
        traffic = THNE_getTraffic(nn);
        YggEvent_addPayload(ev, &traffic, sizeof(double));

        // Append Neigh Type
        is_bi = THNE_isBi(nn);
        YggEvent_addPayload(ev, &is_bi, sizeof(byte));
    }

    #ifdef DEBUG_DISCOVERY
    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(NE_getNeighborID(neigh), id_str);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEW NEIGHBOR", id_str);
    #endif

    // DF_printNeighbors(state);

    state->dirty_neighborhood = true;

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}

void DF_notifyUpdateNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, UPDATE_NEIGHBOR);

    // Append Neighbor ID
    YggEvent_addPayload(ev, NE_getNeighborID(neigh), sizeof(uuid_t));

    // Append MAC addr
    YggEvent_addPayload(ev, NE_getNeighborMAC(neigh), WLAN_ADDR_LEN);

    // Append LQs
    double rx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(ev, &rx_lq, sizeof(double));
    double tx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(ev, &tx_lq, sizeof(double));

    // Append Traffic
    double traffic = NE_getOutTraffic(neigh);
    YggEvent_addPayload(ev, &traffic, sizeof(double));

    // Append Neighbor Type
    byte is_bi = NE_getNeighborType(neigh, &state->current_time) == BI_NEIGH;
    YggEvent_addPayload(ev, &is_bi, sizeof(byte));

    // Append Neighbors
    hash_table* ht = NE_getTwoHopNeighbors(neigh);
    byte n = ht->n_items;
    YggEvent_addPayload(ev, &n, sizeof(byte));

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;

        // Append Neigh ID
        YggEvent_addPayload(ev, THNE_getID(nn), sizeof(uuid_t));

        // Append Neigh LQ
        rx_lq = THNE_getRxLinkQuality(nn);
        YggEvent_addPayload(ev, &rx_lq, sizeof(double));
        tx_lq = THNE_getTxLinkQuality(nn);
        YggEvent_addPayload(ev, &tx_lq, sizeof(double));

        // Append Traffic
        traffic = THNE_getTraffic(nn);
        YggEvent_addPayload(ev, &traffic, sizeof(double));

        // Append Neigh Type
        is_bi = THNE_isBi(nn);
        YggEvent_addPayload(ev, &is_bi, sizeof(byte));
    }

    #ifdef DEBUG_DISCOVERY
    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(NE_getNeighborID(neigh), id_str);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "UPDATE NEIGHBOR", id_str);
    #endif

    // DF_printNeighbors(state);

    state->dirty_neighborhood = true;

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}

void DF_notifyLostNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, LOST_NEIGHBOR);

    // Append Neighbor ID
    YggEvent_addPayload(ev, NE_getNeighborID(neigh), sizeof(uuid_t));

    // Append Neighbor Type
    struct timespec aux;
    copy_timespec(&aux, &state->current_time);
    aux.tv_sec--;
    byte is_bi = NE_getNeighborType(neigh, &aux) == BI_NEIGH;
    YggEvent_addPayload(ev, &is_bi, sizeof(byte));

    // Append Neighbors
    hash_table* ht = NE_getTwoHopNeighbors(neigh);
    byte n = ht->n_items;
    YggEvent_addPayload(ev, &n, sizeof(byte));

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighborEntry* nn = (TwoHopNeighborEntry*)hit->value;

        // Append Neigh ID
        YggEvent_addPayload(ev, THNE_getID(nn), sizeof(uuid_t));

        // Append Neigh Type
        is_bi = THNE_isBi(nn);
        YggEvent_addPayload(ev, &is_bi, sizeof(byte));
    }

    #ifdef DEBUG_DISCOVERY
    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(NE_getNeighborID(neigh), id_str);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "LOST NEIGHBOR", id_str);
    #endif

    // DF_printNeighbors(state);

    state->dirty_neighborhood = true;

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}

void DF_notifyNeighborhood(discovery_framework_state* state) {

    state->dirty_neighborhood = false;

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBORHOOD);

    // Append Neighborhood
    unsigned int size = 0;
    byte* buffer = NULL;
    double out_traffic = DE_getOutTraffic(state->environment);
    NT_serialize(state->neighbors, state->myID, &state->myAddr, out_traffic, &state->current_time, &buffer, &size);
    YggEvent_addPayload(ev, buffer, size);
    free(buffer);

    #ifdef DEBUG_DISCOVERY
    char* str1 = NULL, *str2 = NULL;

    //DF_uponDiscoveryEnvironmentTimer(state); // temp
    NE_print(state->environment, &str1);

    NT_print(state->neighbors, &str2, &state->current_time, state->myID, &state->myAddr, state->my_seq);

    char str3[strlen(str1)+strlen(str2)+1];
    sprintf(str3, "\n%s\n%s", str1, str2);

    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBORHOOD", str3);

    free(str1);
    free(str2);
    #endif

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}

void DF_notifyGenericEvent(char* type, void* buffer, unsigned int size) {
    assert(type);

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, GENERIC_DISCOVERY_EVENT);

    // Append type
    unsigned int str_len = strlen(type);
    YggEvent_addPayload(ev, &str_len, sizeof(unsigned int));
    YggEvent_addPayload(ev, type, str_len);

    // Append Payload
    YggEvent_addPayload(ev, buffer, size);

    #ifdef DEBUG_DISCOVERY
    char str[30];
    sprintf(str, "[%s]", type);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "GENERIC EVENT", str);
    #endif

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}
