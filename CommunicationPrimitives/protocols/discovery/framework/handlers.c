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

static unsigned int compute_missed(unsigned int misses, unsigned long period, struct timespec* exp_time, struct timespec* moment) {

    struct timespec aux;
    subtract_timespec(&aux, exp_time, moment);
    unsigned long remaining = timespec_to_milli(&aux);

    return (unsigned int)(misses - (((double)remaining) / period));
}

static unsigned long compute_next_moment(struct timespec* exp_time, struct timespec* moment, unsigned int misses, unsigned int missed, unsigned long period) {
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

    state->neighbors = newNeighborsTable(state->args->n_buckets, state->args->bucket_duration_s);

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

    // Windows Timer
    genUUID(state->windows_timer_id);
    if(state->args->window_notify_period_s > 0) {
        struct timespec t_ = {0};
        milli_to_timespec(&t_, state->args->window_notify_period_s*1000);
        SetPeriodicTimer(&t_, state->windows_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, WINDOWS_TIMER);
    }

}

void DF_createHello(discovery_framework_state* state, HelloMessage* hello, bool request_replies) {
    assert(hello);

    state->stats.total_hellos++;

    // Increment SEQ
    state->my_seq = inc_seq(state->my_seq, state->args->ignore_zero_seq);

    // Compute Next Hello Period
    struct timespec aux;
    subtract_timespec(&aux, &state->current_time, &state->last_hello_time);
    unsigned long elapsed_time_ms = timespec_to_milli(&aux);
    DA_computeNextHelloPeriod(state->args->algorithm, elapsed_time_ms, state->neighbors);

    // Create Hello
    initHelloMessage(hello,
        state->myID,
        state->my_seq,
        DA_getHelloPeriod(state->args->algorithm),
        computeWindow(NT_getOutTraffic(state->neighbors), &state->current_time, state->args->window_type, "sum", true),
        request_replies
    );

    if( DA_periodicHello(state->args->algorithm) ) {
        // Re-schedule
        scheduleHelloTimer(state, false);
    }

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

    state->stats.total_hacks++;

    if( single ) {
        // Compute Next Hack Period
        struct timespec aux;
        subtract_timespec(&aux, &state->current_time, &state->last_hack_time);
        unsigned long elapsed_time_ms = timespec_to_milli(&aux);
        DA_computeNextHackPeriod(state->args->algorithm, elapsed_time_ms, state->neighbors);
    }

    // Create Standard Hack
    initHackMessage(hack, state->myID,
        NE_getNeighborID(neigh),
        NE_getNeighborSEQ(neigh),
        NE_getRxLinkQuality(neigh),
        NE_getTxLinkQuality(neigh),
        DA_getHackPeriod(state->args->algorithm),
        NE_getOutTraffic(neigh),
        NE_getNeighborType(neigh, &state->current_time));

    if( DA_periodicHack(state->args->algorithm) ) {
        // Re-schedule
        scheduleHackTimer(state, false);
    }

    /*
    #ifdef DEBUG_DISCOVERY
    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse( , id_str);

    char str[200];
    // sprintf(str, "ID=%s SEQ=%hu RX_LQ=%0.2f TX_LQ=%0.2f TYPE=%s PERIOD=%ds", id_str, hack_size);
    str[0] = '\0';
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
        DA_computeNextHackPeriod(state->args->algorithm, elapsed_time_ms, state->neighbors);

        void* iterator = NULL;
        NeighborEntry* current_neigh = NULL;
        HackMessage* current_hack = *hacks;
        while ( (current_neigh = NT_nextNeighbor(state->neighbors, &iterator)) ) {
            DF_createHack(state, current_hack++, current_neigh, false);
        }
    } else {
        // Re-schedule
        scheduleHackTimer(state, false);
    }
}

bool DF_sendMessage(discovery_framework_state* state, HelloMessage* hello, HackMessage* hacks, byte n_hacks, WLANAddr* addr, MessageType msg_type, void* aux_info) {

    bool send = false;

    insertIntoWindow(NT_getOutTraffic(state->neighbors), &state->current_time, 1.0);

    YggMessage msg;
    YggMessage_initBcast(&msg, DISCOVERY_FRAMEWORK_PROTO_ID);

    // Serialize Message
    byte buffer[YGG_MESSAGE_PAYLOAD];
    unsigned short buffer_size = 0;

    send = DA_createDiscoveryMessage(state->args->algorithm, state->myID, &state->current_time, state->neighbors, msg_type, aux_info, hello, hacks, n_hacks, buffer, &buffer_size);

    if( send ) {
        YggMessage_addPayload(&msg, (char*)buffer, buffer_size);

        unsigned short piggyback_size = 0;
        if(addr == NULL) {
            WLANAddr* bcast_addr = getBroadcastAddr();

            pushPayload(&msg, (char*)&piggyback_size, sizeof(piggyback_size), DISCOVERY_FRAMEWORK_PROTO_ID, bcast_addr);

            free(bcast_addr);
        } else {
            pushPayload(&msg, (char*)&piggyback_size, sizeof(piggyback_size), DISCOVERY_FRAMEWORK_PROTO_ID, addr);
        }

        // Insert into dispatcher queue
        DF_dispatchMessage(state->dispatcher_queue, &msg);

        state->stats.discovery_messages++;
    } else {
        if(hello != NULL) {
            state->my_seq = dec_seq(state->my_seq, state->args->ignore_zero_seq);
        }
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

    if(neighbor_change) {
        scheduleNeighborChange(state, false, false, false, true);
    }
}

void scheduleHelloTimer(discovery_framework_state* state, bool now) {

    if( DA_periodicHello(state->args->algorithm) ) {

        unsigned long jitter = (unsigned long)(randomProb()*state->args->max_jitter_ms);

        unsigned long t = now ? jitter : DA_getHelloPeriod(state->args->algorithm)*1000 - jitter - state->args->period_margin_ms;

        struct timespec t_ = {0};
        milli_to_timespec(&t_, t);

        if( state->hello_timer_active ) {
            /*
            struct timespec remaining;
            subtract_timespec(&remaining, &state->next_hello_time, &state->current_time);

            if( compare_timespec(&remaining, &t_) > 0 ) {
                // Re-schedule
                CancelTimer(state->hello_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID);

                SetTimer(&t_, state->hello_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HELLO_TIMER);

                add_timespec(&state->next_hello_time, &t_, &state->current_time);
            } else {
                // Just update the dates
                add_timespec(&state->next_hello_time, &t_, &state->current_time);
            }
            */
        } else {
            state->hello_timer_active = true;

            SetTimer(&t_, state->hello_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HELLO_TIMER);

            add_timespec(&state->next_hello_time, &t_, &state->current_time);
        }
    }
}

void scheduleHackTimer(discovery_framework_state* state, bool now) {

    if( DA_periodicHack(state->args->algorithm) ) {

        unsigned long jitter = (unsigned long)(randomProb()*state->args->max_jitter_ms);

        unsigned long t = now ? jitter : DA_getHackPeriod(state->args->algorithm)*1000 - jitter - state->args->period_margin_ms;

        struct timespec t_ = {0};
        milli_to_timespec(&t_, t);

        if( state->hack_timer_active ) {
            /*
            struct timespec remaining;
            subtract_timespec(&remaining, &state->next_hack_time, &state->current_time);

            if( compare_timespec(&remaining, &t_) > 0 ) {
                // Re-schedule
                CancelTimer(state->hack_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID);

                SetTimer(&t_, state->hack_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HACK_TIMER);

                add_timespec(&state->next_hack_time, &t_, &state->current_time);
            } else {
                // Just update the dates
                add_timespec(&state->next_hack_time, &t_, &state->current_time);
            }
            */
        } else {
            state->hack_timer_active = true;

            SetTimer(&t_, state->hack_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, HACK_TIMER);

            add_timespec(&state->next_hack_time, &t_, &state->current_time);
        }
    }
}

void DF_uponHelloTimer(discovery_framework_state* state, bool periodic, bool send_hack) {
    assert( state->hello_timer_active );

    if( DA_periodicHello(state->args->algorithm) ) {

        if( compare_timespec(&state->current_time, &state->next_hello_time) >= 0 ) {

            state->hello_timer_active = false;
            copy_timespec(&state->last_hello_time, &state->current_time);

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
            if( periodic ) {
                sent = DF_sendMessage(state, &hello, hacks, n_hacks, NULL, PERIODIC_MSG, NULL);
            } else {
                void* aux_info = &state->neighbor_change_summary;
                sent = DF_sendMessage(state, &hello, hacks, n_hacks, NULL, NEIGHBOR_CHANGE_MSG, aux_info);
            }

            if( sent ) {
                if(piggyback_hack) {
                    state->stats.piggybacked_hacks += n_hacks;
                }
            }

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

            state->hack_timer_active = false;
            copy_timespec(&state->last_hack_time, &state->current_time);

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
            NE_setRxLinkQuality(neigh, new_rx_lq);

            double lq_delta = fabs(old_rx_lq - new_rx_lq);
            if( lq_delta > 0.0 ) {
                summary->updated_quality = true;

                if( lq_delta >= state->args->lq_threshold ) {
                    summary->updated_quality_threshold = true;
                }
            }
        }

        unsigned long next_hello_miss =  compute_next_moment(rx_exp_time, &state->current_time, hello_misses, missed_hellos, hello_period);

        // Check if tx not expired
        if( compare_timespec(tx_exp_time, &state->current_time) > 0 ) {
            unsigned long hack_period = NE_getNeighborHackPeriod(neigh)*1000;
            unsigned int hack_misses = state->args->hack_misses;

            unsigned int missed_hacks = compute_missed(hack_misses, hack_period, tx_exp_time, &state->current_time);

            unsigned int prev_missed_hacks = compute_missed(hack_misses, hack_period, tx_exp_time, &last_neighbor_timer);

            if( missed_hacks > prev_missed_hacks ) {
                state->stats.missed_hacks++;

                summary->missed_hacks++;

                // TODO: actualizar também a qualidade inversa (como se fosse a qualidade normal) quando se perdem hacks? --> não porque não receber hacks não quer dizer que os hellos não estejam a chegar ao outro lado

                // atualizar a qualidade normal como se se perdessem hellos?

                // não fazer nada porque os hellos já estão a castigar as perdas?
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
            TwoHopNeighbor* nn = (TwoHopNeighbor*)hit->value;
            if( compare_timespec(&nn->expiration, &state->current_time) < 0 ) {
                hash_table_remove(ht, nn->id);
                free(nn);
                free(hit);
                summary->deleted_2hop++;
            } else {
                if( first || compare_timespec(&nn->expiration, &min_exp) < 0 ) {
                    first = false;
                    copy_timespec(&min_exp, &nn->expiration);
                }
            }
        }
        // printf("deleted %u 2-hop neighs\n", deleted_two_hop);

        //if( deleted_two_hop > 0 ) {
            // printf("lost 2-hop neigh(s)\n");
            //scheduleNeighborChange(state, 2);
        //}

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

    if( summary->updated_neighbor || summary->deleted_2hop > 0 ) {
        DF_notifyUpdateNeighbor(state, neigh);

        if( summary->lost_bi || summary->updated_quality_threshold || summary->deleted_2hop > 0 ) {
            scheduleNeighborChange(state, NULL, NULL, summary, false);
        }
    }

    if( summary->lost_neighbor ) {
        DF_notifyLostNeighbor(state, neigh);

        scheduleNeighborChange(state, NULL, NULL, summary, false);

        // TODO: count as instability?
    }

    if( !summary->removed ) {
        // printf("next timer %lu\n", next_timer);

        struct timespec t;
        milli_to_timespec(&t, next_timer);
        SetTimer(&t, NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_TIMER);
    }

    free(summary);

    return summary->removed;
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

    bool other = state->neighbor_change_summary.other;
    bool new_neighbor = state->neighbor_change_summary.new_neighbor;
    bool lost_neighbor = state->neighbor_change_summary.lost_neighbor;
    bool updated_neighbor = state->neighbor_change_summary.updated_neighbor;
    bool new_2hop_neighbor = state->neighbor_change_summary.added_two_hop_neighbor;
    bool lost_2hop_neighbor = state->neighbor_change_summary.lost_two_hop_neighbor;
    bool updated_2hop_neighbor = state->neighbor_change_summary.updated_two_hop_neighbor;

    DiscoveryAlgorithm* alg = state->args->algorithm;

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

    if( send_hello ) {
        if( compare_timespec(&state->set_neighbor_change_time, &state->last_hello_time) > 0 ) {
            CancelTimer(state->hello_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID);
            copy_timespec(&state->next_hello_time, &state->current_time);
            DF_uponHelloTimer(state, false, send_hack);
        }
    }

    if( !send_hello && send_hack ) {
        if( compare_timespec(&state->set_neighbor_change_time, &state->last_hack_time) > 0 ) {
            CancelTimer(state->hack_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID);
            copy_timespec(&state->next_hack_time, &state->current_time);
            DF_uponHackTimer(state, false);
        }
    }

    state->neighbor_change_timer_active = false;
    memset(&state->neighbor_change_summary, 0, sizeof(state->neighbor_change_summary));

    // Log
    #ifdef DEBUG_DISCOVERY
    char str[200];
    sprintf(str, "HELLO=[%s] HACK=[%s]", (send_hello ? "yes" : "no"), (send_hack ? "yes" : "no"));
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEIGHBOR CHANGE TIMER", str);
    #endif
}

void scheduleNeighborChange(discovery_framework_state* state, HelloDeliverSummary* hello_summary, HackDeliverSummary* hack_summary, NeighborTimerSummary* neighbor_timer_summary, bool other) {

    // state->stats.updated_neighbors++;

    DiscoveryAlgorithm* alg = state->args->algorithm;

    // New schedule
    if( !state->neighbor_change_timer_active ) {
        bool other = state->neighbor_change_summary.other;
        bool new_neighbor = state->neighbor_change_summary.new_neighbor;
        bool lost_neighbor = state->neighbor_change_summary.lost_neighbor;

        bool updated_neighbor = state->neighbor_change_summary.updated_neighbor;

        bool new_2hop_neighbor = state->neighbor_change_summary.added_two_hop_neighbor;
        bool lost_2hop_neighbor = state->neighbor_change_summary.lost_two_hop_neighbor;
        bool updated_2hop_neighbor = state->neighbor_change_summary.updated_two_hop_neighbor;

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

            memset(&state->neighbor_change_summary, 0, sizeof(state->neighbor_change_summary));

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
    if( hello_summary ) {
        state->neighbor_change_summary.new_neighbor |= hello_summary->new_neighbor;
        state->neighbor_change_summary.updated_neighbor |= hello_summary->updated_neighbor;
        state->neighbor_change_summary.rebooted |= hello_summary->rebooted;
        state->neighbor_change_summary.hello_period_changed |= hello_summary->period_changed;
        state->neighbor_change_summary.updated_quality |= hello_summary->updated_quality;
        state->neighbor_change_summary.updated_quality_threshold |= hello_summary->updated_quality_threshold;
        // state->neighbor_change_summary.updated_traffic |= hello_summary->updated_traffic;
        // state->neighbor_change_summary.updated_traffic_threshold |= hello_summary->updated_traffic_threshold;
        // state->neighbor_change_summary.missed_hellos += hello_summary->misseds_hellos;
    }

    if( hack_summary ) {
        state->neighbor_change_summary.updated_neighbor |= hack_summary->updated_neighbor;
        // state->neighbor_change_summary.missed_hacks += hack_summary->missed_hacks;
        state->neighbor_change_summary.became_bi |= hack_summary->became_bi;
        state->neighbor_change_summary.lost_bi |= hack_summary->lost_bi;
        state->neighbor_change_summary.hack_period_changed |= hack_summary->period_changed;
        state->neighbor_change_summary.updated_quality |= hack_summary->updated_quality;
        state->neighbor_change_summary.updated_quality_threshold |= hack_summary->updated_quality_threshold;
        state->neighbor_change_summary.updated_two_hop_neighbor |= hack_summary->updated_two_hop_neighbor;
        state->neighbor_change_summary.added_two_hop_neighbor |= hack_summary->added_two_hop_neighbor;
        state->neighbor_change_summary.lost_two_hop_neighbor |= hack_summary->lost_two_hop_neighbor;
    }

    if( neighbor_timer_summary ) {
        state->neighbor_change_summary.updated_neighbor |= neighbor_timer_summary->updated_neighbor;
        state->neighbor_change_summary.lost_neighbor |= neighbor_timer_summary->lost_neighbor;
        state->neighbor_change_summary.removed |= neighbor_timer_summary->removed;
        state->neighbor_change_summary.lost_bi |= neighbor_timer_summary->lost_bi;
        state->neighbor_change_summary.updated_quality |= neighbor_timer_summary->updated_quality;
        state->neighbor_change_summary.updated_quality_threshold |= neighbor_timer_summary->updated_quality_threshold;
        state->neighbor_change_summary.deleted_2hop += neighbor_timer_summary->deleted_2hop;
        //state->neighbor_change_summary.missed_hellos  += neighbor_timer_summary->missed_hellos;
        //state->neighbor_change_summary.missed_hacks  += neighbor_timer_summary->missed_hacks;
    }

    //if( other ) {
        state->neighbor_change_summary.other |= other;
    //}

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

    summary->updated_two_hop_neighbor = false;
    summary->added_two_hop_neighbor = false;
    summary->lost_two_hop_neighbor = false;

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

                char str[200];
                sprintf(str, "%s", id_str);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REBOOTED", str);
                #endif
            }
        }
    }

    if( neigh ) {
        NE_setNeighborRxExpTime(neigh, &rx_exp_time);

        unsigned int prev_missed_hellos = compute_missed(state->args->hello_misses, NE_getNeighborHelloPeriod(neigh)*1000, NE_getNeighborRxExpTime(neigh), NE_getLastNeighborTimer(neigh));

        int seq_cmp = compare_seq(hello->seq, NE_getNeighborSEQ(neigh), state->args->ignore_zero_seq);
        assert(seq_cmp >= 0);

        summary->missed_hellos = (seq_cmp - 1 - prev_missed_hellos);
        assert(summary->missed_hellos >= 0);

        state->stats.missed_hellos += summary->missed_hellos;

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
    NE_setRxLinkQuality(neigh, new_rx_lq);

    double lq_delta = fabs(old_rx_lq - new_rx_lq);
    if( lq_delta > 0.0 ) {
        summary->updated_quality = true;

        if( lq_delta >= state->args->lq_threshold ) {
            summary->updated_quality_threshold = true;
        }
    }

    // Update traffic
    double traffic_delta = fabs(NE_getOutTraffic(neigh) - hello->traffic);
    if( traffic_delta > 0.0 ) {
        summary->updated_traffic = true;

        if( traffic_delta >= state->args->traffic_threshold ) {
            summary->updated_traffic_threshold = true;
        }
    }
    NE_setOutTraffic(neigh, hello->traffic);

    // Re-schedule neighbor timer
    if( summary->period_changed ) {
        CancelTimer(NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID);

        bool removed = DF_uponNeighborTimer(state, neigh);
        assert(!removed);
    }

    // Notify
    if( summary->new_neighbor ) {
        DF_notifyNewNeighbor(state, neigh);

        scheduleNeighborChange(state, summary, NULL, NULL, false);

        insertIntoWindow(NT_getInstability(state->neighbors), &state->current_time, 1.0);
    } else if( summary->updated_quality || summary->updated_traffic || summary->rebooted ) {
        summary->updated_neighbor = true;
        DF_notifyUpdateNeighbor(state, neigh);

        if( summary->updated_quality_threshold || summary->updated_traffic_threshold || summary->rebooted ) {
            scheduleNeighborChange(state, summary, NULL, NULL, false);
        }

        // insertIntoWindow(NT_getStability(state->neighbors), &state->current_time);
        // TODO: aqui também?
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

            // If is a positive HACK
            if( hack->neigh_type != LOST_NEIGH ) {
                summary->positive_hack = true;

                // Check if the hack is meant for me
                if( uuid_compare(state->myID, hack->dest_process_id) == 0 ) {

                    // Check freshness of the hack
                    int seq_cmp = compare_seq(hack->seq, NE_getNeighborHSEQ(neigh), state->args->ignore_zero_seq);
                    //assert(seq_cmp >= 0);

                    unsigned int prev_missed_hacks = compute_missed(state->args->hack_misses, NE_getNeighborHackPeriod(neigh)*1000, NE_getNeighborTxExpTime(neigh), NE_getLastNeighborTimer(neigh));
                    summary->missed_hacks = (seq_cmp - 1 - prev_missed_hacks);
                    state->stats.missed_hacks += summary->missed_hacks;

                    summary->new_hack = seq_cmp > 0;
                    summary->repeated_yet_fresh_hack = seq_cmp == 0 && hack->seq == dec_seq(state->my_seq, state->args->ignore_zero_seq);
                    if( summary->new_hack || summary->repeated_yet_fresh_hack ) {
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

                            // scheduleNeighborChange(state, false, false, true, false);
                        }

                        // Update Tx expiration timestamp
                        NE_setNeighborTxExpTime(neigh, &hack_exp_time);

                        // Update Link Quality
                        double old_tx_lq = NE_getTxLinkQuality(neigh);
                        double new_tx_lq = hack->rx_lq;
                        NE_setTxLinkQuality(neigh, new_tx_lq);

                        double lq_delta = fabs(old_tx_lq - new_tx_lq);
                        if( lq_delta > 0.0 ) {
                            summary->updated_neighbor = true;
                            summary->updated_quality = true;

                            if( lq_delta >= state->args->lq_threshold ) {
                                // scheduleNeighborChange(state, false, false, true, false);

                                summary->updated_quality_threshold = true;
                            }
                        }

                        // Check if HACK period changed
                        if( NE_getNeighborHackPeriod(neigh) != hack->period ) {
                            NE_setNeighborHackPeriod(neigh, hack->period);

                            summary->period_changed = true;
                        }
                    } else {
                        // stale hack
                    }
                }

                // Update 2-hop neighborhood (Neighbor's neighbors)
                TwoHopNeighbor* nn = NE_getTwoHopNeighbor(neigh, hack->dest_process_id);
                if( nn ) {
                    int seq_cmp = compare_seq(hack->seq, nn->hseq, state->args->ignore_zero_seq);

                    // if fresh hack
                    if( seq_cmp >= 0 ) {
                        nn->hseq = hack->seq;

                        bool is_symmetric = hack->neigh_type == BI_NEIGH;
                        if( nn->is_symmetric != is_symmetric ) {
                            nn->is_symmetric = is_symmetric;

                            summary->updated_two_hop_neighbor = true;
                            //summary->updated_neighbor = true;
                        }

                        double rx_lq_delta = fabs(nn->rx_lq - hack->rx_lq);
                        double tx_lq_delta = fabs(nn->tx_lq - hack->tx_lq);
                        if( rx_lq_delta > 0.0 || tx_lq_delta > 0.0 ) {
                            summary->updated_two_hop_neighbor = true;
                            //summary->updated_neighbor = true;

                            /*
                            if( rx_lq_delta >= state->args->lq_threshold || tx_lq_delta >= state->args->lq_threshold ) {

                            }
                            */
                        }
                        nn->rx_lq = hack->rx_lq;
                        nn->tx_lq = hack->tx_lq;

                        double traffic_delta = fabs(nn->traffic - hack->traffic);
                        if( traffic_delta > 0.0 ) {
                            summary->updated_two_hop_neighbor = true;
                            //summary->updated_neighbor = true;

                            /*
                            if( traffic_delta >= state->args->traffic_threshold ) {
                                // summary->updated_traffic_threshold = true;
                            }
                            */
                        }
                        nn->traffic = hack->traffic;

                        copy_timespec(&nn->expiration, &hack_exp_time);
                    }

                } else {
                    // Insert new 2-hop neighbor
                    TwoHopNeighbor* nn = newNeighTwoHopNeighbor(hack->dest_process_id, hack->seq, hack->neigh_type == BI_NEIGH, hack->rx_lq, hack->tx_lq, hack->traffic, &hack_exp_time);

                    TwoHopNeighbor* aux = NE_addTwoHopNeighbor(neigh, nn);
                    assert(aux == NULL);

                    //summary->updated_neighbor = true;
                    summary->added_two_hop_neighbor = true;
                }
            }

            // If is a negative HACK
            else {
                summary->positive_hack = false;

                // Check if the hack is meant for me
                if( uuid_compare(state->myID, hack->dest_process_id) == 0 ) {

                    // Check freshness of the hack
                    int seq_cmp = compare_seq(hack->seq, NE_getNeighborHSEQ(neigh), state->args->ignore_zero_seq);

                    summary->new_hack = seq_cmp > 0;
                    summary->repeated_yet_fresh_hack = seq_cmp == 0 && hack->seq == dec_seq(state->my_seq, state->args->ignore_zero_seq);
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

                        // scheduleNeighborChange(state, false, false, true, false);

                        // summary->updated_neighbor = true;
                    }
                }

                // Update 2-hop neighborhood
                TwoHopNeighbor* nn = NE_getTwoHopNeighbor(neigh, hack->dest_process_id);
                if( nn ) {
                    int seq_cmp = compare_seq(hack->seq, nn->hseq, state->args->ignore_zero_seq);

                    // if fresh hack
                    if( seq_cmp >= 0 ) {
                        // Remove from 2-hop neighborhood
                        TwoHopNeighbor* removed = NE_removeTwoHopNeighbor(neigh, hack->dest_process_id);
                        if(removed) {
                            free(removed);
                            summary->lost_two_hop_neighbor = true;
                            //summary->updated_neighbor = true;
                        }
                    }
                }
            }
    } else {
        // The neighbor is not know. Ignore the HACK
    }

    summary->updated_neighbor = summary->became_bi || summary->lost_bi || summary->updated_quality;

    if( summary->updated_neighbor || summary->updated_two_hop_neighbor || summary->added_two_hop_neighbor || summary->lost_two_hop_neighbor ) {
        DF_notifyUpdateNeighbor(state, neigh);

        if( summary->became_bi || summary->lost_bi || summary->updated_quality_threshold || summary->updated_two_hop_neighbor || summary->added_two_hop_neighbor || summary->lost_two_hop_neighbor ) {
            scheduleNeighborChange(state, NULL, summary, NULL, false);
        }
    }

    // Re-schedule neigh timer
    CancelTimer(NE_getNeighborID(neigh), DISCOVERY_FRAMEWORK_PROTO_ID);
    bool removed = DF_uponNeighborTimer(state, neigh);
    assert(!removed);

    return summary;
}

void DF_piggybackDiscovery(discovery_framework_state* state, YggMessage* msg) {

    insertIntoWindow(NT_getOutTraffic(state->neighbors), &state->current_time, 1.0);

    unsigned short piggyback_size = 0;
    byte buffer[YGG_MESSAGE_PAYLOAD];

    WLANAddr* bcast_addr = getBroadcastAddr();
    WLANAddr* dest_addr = &msg->destAddr;
    bool is_unicast_addr = memcmp(dest_addr->data, bcast_addr->data, WLAN_ADDR_LEN) == 0;
    WLANAddr* addr = is_unicast_addr ? dest_addr : bcast_addr;

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
                if( memcmp(NE_getNeighborMAC(current_neigh)->data, dest_addr->data, WLAN_ADDR_LEN) == 0 ) {
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

    bool send = false;
    if( hello || hacks ) {
        send = DA_createDiscoveryMessage(state->args->algorithm, state->myID, &state->current_time, state->neighbors, PIGGYBACK_MSG, &msg, hello, hacks, n_hacks, buffer + sizeof(piggyback_size), &piggyback_size);
    }

    if( send ) {
        memcpy(buffer, &piggyback_size, sizeof(piggyback_size));

        // TODO: try to leverage promiscuous mode in the future

        pushPayload(msg, (char*)buffer, sizeof(piggyback_size) + piggyback_size, DISCOVERY_FRAMEWORK_PROTO_ID, addr);

        if( piggyback_hello ) {
            state->stats.piggybacked_hellos++;
        }

        if( piggyback_hack ) {
            state->stats.piggybacked_hacks += n_hacks;
        }
    } else {
        if( hello != NULL ) {
            state->my_seq = dec_seq(state->my_seq, state->args->ignore_zero_seq);
        }

        unsigned short aux = 0;
        pushPayload(msg, (char*)&aux, sizeof(aux), DISCOVERY_FRAMEWORK_PROTO_ID, addr);
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

void DF_uponStatsRequest(discovery_framework_state* state, YggRequest* req) {
    unsigned short dest = req->proto_origin;
	YggRequest_init(req, DISCOVERY_FRAMEWORK_PROTO_ID, dest, REPLY, REQ_DISCOVERY_FRAMEWORK_STATS);

	YggRequest_addPayload(req, (void*)&state->stats, sizeof(discovery_stats));

	deliverReply(req);

	YggRequest_freePayload(req);
}

void DF_uponWindowsTimer(discovery_framework_state* state) {

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, WINDOWS_EVENT);

    double instability = computeWindow(NT_getInstability(state->neighbors), &state->current_time, state->args->window_type, "sum", true);

    double out_traffic = computeWindow(NT_getOutTraffic(state->neighbors), &state->current_time, state->args->window_type, "sum", true);

    double in_traffic = 0.0;
    // double misses = 0.0;

    void* iterator = NULL;
    NeighborEntry* current_neigh = NULL;
    while( (current_neigh = NT_nextNeighbor(state->neighbors, &iterator)) ) {
        in_traffic += NE_getOutTraffic(current_neigh);

        //computeWindow(, &state-> current_time, state->args->window_type, "sum");


        // misses += computeWindow(NE_getMisses(current_neigh), &state-> current_time, state->args->window_type); // TODO: param
    }

    YggEvent_addPayload(ev, &instability, sizeof(double));
    // YggEvent_addPayload(ev, &misses, sizeof(double));
    YggEvent_addPayload(ev, &in_traffic, sizeof(double));
    YggEvent_addPayload(ev, &out_traffic, sizeof(double));

    #ifdef DEBUG_DISCOVERY
    char str[100];
    //sprintf(str, "stability: %.2f misses: %.2f in_traffic: %.2f out_traffic: %.2f", stability, misses, in_traffic, out_traffic);
    sprintf(str, "stability: %.2f in_traffic: %.2f out_traffic: %.2f", instability, in_traffic, out_traffic);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "WINDOWS", str);
    #endif

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}

void DF_printNeighbors(discovery_framework_state* state) {
    char* str = NULL;
    printf("\n%s\n", NT_print(state->neighbors, &str, &state->current_time, state->args->window_type, state->myID, &state->myAddr, state->my_seq));
    free(str);
}

void DF_notifyNewNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_FOUND);

    // Append Neighbor ID
    YggEvent_addPayload(ev, NE_getNeighborID(neigh), sizeof(uuid_t));

    // Append MAC addr
    YggEvent_addPayload(ev, NE_getNeighborMAC(neigh), WLAN_ADDR_LEN);

    // Append LQs
    double rx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(ev, &rx_lq, sizeof(double));
    double tx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(ev, &tx_lq, sizeof(double));

    #ifdef DEBUG_DISCOVERY
    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(NE_getNeighborID(neigh), id_str);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEW NEIGHBOR", id_str);

    DF_printNeighbors(state);
    #endif

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}

void DF_notifyUpdateNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_UPDATE);

    // Append Neighbor ID
    YggEvent_addPayload(ev, NE_getNeighborID(neigh), sizeof(uuid_t));

    // Append MAC addr
    YggEvent_addPayload(ev, NE_getNeighborMAC(neigh), WLAN_ADDR_LEN);

    // Append LQs
    double rx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(ev, &rx_lq, sizeof(double));
    double tx_lq = NE_getRxLinkQuality(neigh);
    YggEvent_addPayload(ev, &tx_lq, sizeof(double));

    // Append Neighbor Type
    byte is_symmetric = NE_getNeighborType(neigh, &state->current_time) == BI_NEIGH;
    YggEvent_addPayload(ev, &is_symmetric, sizeof(byte));

    // Append Neighbors
    hash_table* ht = NE_getTwoHopNeighbors(neigh);
    byte n = ht->n_items;
    YggEvent_addPayload(ev, &n, sizeof(byte));

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
        TwoHopNeighbor* nn = (TwoHopNeighbor*)hit->value;

        // Append Neigh ID
        YggEvent_addPayload(ev, nn->id, sizeof(uuid_t));

        // Append Neigh LQ
        YggEvent_addPayload(ev, &nn->rx_lq, sizeof(double));
        YggEvent_addPayload(ev, &nn->tx_lq, sizeof(double));

        // Append Neigh Type
        is_symmetric = nn->is_symmetric;
        YggEvent_addPayload(ev, &is_symmetric, sizeof(byte));

        // TODO: append traffic?
    }

    #ifdef DEBUG_DISCOVERY
    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(NE_getNeighborID(neigh), id_str);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "UPDATE NEIGHBOR", id_str);

    DF_printNeighbors(state);
    #endif

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}

void DF_notifyLostNeighbor(discovery_framework_state* state, NeighborEntry* neigh) {
    assert(neigh);

    YggEvent* ev = malloc(sizeof(YggEvent));
    YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_LOST);

    // Append Neighbor ID
    YggEvent_addPayload(ev, NE_getNeighborID(neigh), sizeof(uuid_t));

    #ifdef DEBUG_DISCOVERY
    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(NE_getNeighborID(neigh), id_str);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "LOST NEIGHBOR", id_str);

    DF_printNeighbors(state);
    #endif

    deliverEvent(ev);
    YggEvent_freePayload(ev);
    free(ev);
}

















/*

void scheduleHeartbeat(discovery_framework_state* state, bool now, unsigned long jitter) {

    if( state->heartbeat_period_s > 0 ) {
        unsigned long t = now ? 0 : state->heartbeat_period_s*1000;

        unsigned long t2 = state->args->max_announce_jitter_ms > t + jitter ? 0 : t + jitter - state->args->max_announce_jitter_ms;

        struct timespec t_ = {0};
        milli_to_timespec(&t_, t2);

        if(state->hb_timer_is_expired) {
                    SetTimer(&t_, state->heartbeat_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, -1);
                    state->hb_timer_is_expired = false;
        }

        add_timespec(&state->next_heartbeat_date, &t_, &state->current_time);
    }
}

void uponHeartbeatTimer(discovery_framework_state* state) {

    if( state->heartbeat_period_s == 0 ) {
        printf("ERROR: heartbeat_period_s == 0!\n");
        return;
    }

    if( compare_timespec(&state->next_heartbeat_date, &state->current_time) <= 0 ) {
        // Timer Expired
        state->hb_timer_is_expired = true;

        // ANNOUNCE
        unsigned long t = scheduleAnnounce(state, false);
        if( imSubscribedTo(state->args->algorithm, DISCOVERY_FRAMEWORK_PROTO_ID, HEARTBEAT_EVENT) ) {
            YggEvent* ev = malloc(sizeof(YggEvent));
            YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, HEARTBEAT_EVENT);
            list_add_item_to_tail(state->triggered_events, ev);
        }

        scheduleHeartbeat(state, false, t);
    } else {
        // Re-schedule
        struct timespec remaining;
        subtract_timespec(&remaining, &state->next_heartbeat_date, &state->current_time);

        SetTimer(&remaining, state->heartbeat_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, -1);

        assert(state->hb_timer_is_expired == false);
    }
}

void changeHeartbeatPeriod(discovery_framework_state* state, unsigned long new_heartbeat_period_s) {

    unsigned long old_heartbeat_period_s = state->heartbeat_period_s;

    if( old_heartbeat_period_s > 0 ) {
        assert( state->hb_timer_is_expired == false );

        CancelTimer(state->heartbeat_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID);
        memset(&state->next_heartbeat_date, 0 , sizeof(struct timespec));
    } else {
        assert( state->hb_timer_is_expired == true );
    }

    state->heartbeat_period_s = new_heartbeat_period_s;

    scheduleHeartbeat(state, true, 0L);

    // TODO: LOG
}

void piggybackHeartbeat(discovery_framework_state* state, YggMessage* msg, bool piggybacked) {
    // Update last heartbeat timestamp
    copy_timespec(&state->last_heartbeat_date, &state->current_time);

    scheduleHeartbeat(state, false, 0L);

    HeartbeatHeader hb;
    if(state->args->hb_only_in_announces && piggybacked) {
        initHeartbeatHeader(&hb, state->myID, 0, 0);
    } else {
        initHeartbeatHeader(&hb, state->myID, ++(state->my_seq), state->heartbeat_period_s);
    }
    pushHeartbeatHeader(msg, &hb);

    insertIntoWindow(state->out_traffic, &state->current_time);

    if(piggybacked)
		state->stats.piggybacked_heartbeats++;
}


*/


/*
void processHeartbeat(discovery_framework_state* state, HeartbeatHeader* hb, YggMessage* announce, bool has_announce) {

    bool process = state->args->hb_only_in_announces ? has_announce : true;
    if(!process) {
        return;
    }

    NeighborEntry* neigh = NT_getNeighbor(state->neighbors, hb->process_id);
    bool is_new_neigh = false;
    bool updated_neigh = false;

    struct timespec asym_exp_time = {0}, sym_exp_time = {0}, removal_time = {0};

    milli_to_timespec(&asym_exp_time, state->args->hb_misses * hb->period*1000);
    add_timespec(&asym_exp_time, &asym_exp_time, &state->current_time);

    copy_timespec(&sym_exp_time, &state->current_time);
    sym_exp_time.tv_sec--; // Set as expired

    milli_to_timespec(&removal_time, state->args->neigh_hold_time_s*1000);
    add_timespec(&removal_time, &removal_time, &asym_exp_time);

    int lost_messages = 0;

    // New Neighbor
    if(neigh == NULL) {
        neigh = newNeighEntry(&announce->srcAddr, hb->process_id, hb->seq, &asym_exp_time, &sym_exp_time, &removal_time, announce, state->args->window_duration_s);
        NT_addNeighbor(state->neighbors, neigh);

        is_new_neigh = true;

        lost_messages = -1; // Initial value
    } else {
        if(NE_isDeleted(neigh)) {
            NE_setDeleted(neigh, false);
            // lost_messages = -1;
            is_new_neigh = true;
        }

        NE_setNeighborAsymTime(neigh, &asym_exp_time);
        NE_setNeighborRemovalTime(neigh, &removal_time);
        NE_setNeighborLastAnnounce(neigh, announce);

        lost_messages = (hb->seq - NE_getNeighborSEQ(neigh) - 1);
        assert(lost_messages >= 0);

        NE_setNeighborSEQ(neigh, hb->seq);
    }

    insertIntoWindow(NE_getInTraffic(neigh), &state->current_time);
    for(int i = 0; i < lost_messages; i++) {
        insertIntoWindow(NE_getMisses(neigh), &state->current_time);
    }

    double old_neigh_lq = NE_getNeighLinkQuality(neigh);

    if(has_announce) {
            // Process Announce
            bool is_bi = false, was_bi = NE_getNeighborType(neigh, &state->current_time) == BI_NEIGH;
            bool updated_neighs = false;

            bool reply = processDiscoveryAnnounce(state->args->algorithm, state->myID, &state->current_time, state->neighbors, neigh, is_new_neigh, (unsigned char*)announce->data, announce->dataLen, &is_bi, &updated_neighs);

            updated_neigh = updated_neighs || (is_bi && !was_bi && !is_new_neigh);

            if(is_bi) {
                NE_setNeighborSymTime(neigh, &asym_exp_time); // TODO: SYM = ASYM ?
            } else {
                NE_setNeighborSymTime(neigh, &sym_exp_time); // TODO: SYM = expired ?
            }

            if(reply) {
                // TODO:
                YggEvent* ev = malloc(sizeof(YggEvent));
                YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, REPLY_EVENT);
                YggEvent_addPayload(ev, announce, sizeof(YggMessage));
                triggerDiscoveryEvent(state, ev, true, true);
            }
    }

    // Smart GC
    if( is_new_neigh && !state->gc_active ) {
        state->gc_active = true;

        struct timespec t;
        subtract_timespec(&t, NE_getNeighborAsymTime(neigh), &state->current_time);

        SetTimer(&t, state->gc_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, -1);
    }

    // Link Quality
    double old_my_lq = NE_getMyLinkQuality(neigh);
    double my_lq = DA_computeLinkUniQuality(state->args->algorithm, NE_getMyLinkQuality(neigh), lost_messages);
    NE_setMyLinkQuality(neigh, my_lq);
    double neigh_lq = NE_getNeighLinkQuality(neigh);
    double old_bi_lq = NE_getBiLinkQuality(neigh);
    double bi_lq = DA_computeLinkBiQuality(state->args->algorithm, my_lq, neigh_lq);
    NE_setBiLinkQuality(neigh, bi_lq);
    printf("LQ: %f %f %f\n", my_lq, neigh_lq, bi_lq);

    // Check if the new quality changed too much
    bool updated_quality = fabs(old_bi_lq - bi_lq) >= state->args->lq_threshold \
                        || fabs(old_my_lq - my_lq) >= state->args->lq_threshold \
                        || fabs(old_neigh_lq - neigh_lq) >= state->args->lq_threshold;

    // Log
    if(is_new_neigh) {
        notify_NewNeighbor(state, neigh);
        insertIntoWindow(state->stability, &state->current_time);
    } else if(updated_neigh || updated_quality) {
        printf("updated_neigh %d updated_quality %d\n", updated_neigh, updated_quality);
        notify_UpdateNeighbor(state, neigh);
        insertIntoWindow(state->stability, &state->current_time);
    }
}

*/


/*
void processHeartbeat(discovery_framework_state* state, HeartbeatHeader* hb, YggMessage* announce, bool has_announce) {

    DiscoveryType d_type = getDiscoveryType(state->args->algorithm);

    bool process = (d_type == PASSIVE_DISCOVERY) \
                || (d_type == HYBRID_DISCOVERY) \
                || (d_type == ACTIVE_DISCOVERY && (has_announce || state->args->process_hb_on_active));

    if(!process)
        return;

    NeighborEntry* neigh = NT_getNeighbor(state->neighbors, hb->process_id);
    bool is_new_neigh = false;
    bool updated_neigh = false;

    struct timespec asym_exp_time = {0}, sym_exp_time = {0}, removal_time = {0};

    milli_to_timespec(&asym_exp_time, state->args->neigh_validity_s*1000);
    add_timespec(&asym_exp_time, &asym_exp_time, &state->current_time);

    copy_timespec(&sym_exp_time, &state->current_time);
    sym_exp_time.tv_sec--;

    milli_to_timespec(&removal_time, state->args->neigh_hold_time_s*1000);
    add_timespec(&removal_time, &removal_time, &asym_exp_time);

    int lost_messages = 0;

    // New Neighbor
    if(neigh == NULL) {
        neigh = newNeighEntry(&announce->srcAddr, hb->process_id, hb->seq, &asym_exp_time, &sym_exp_time, &removal_time, 0, NULL, announce, state->args->window_duration_s);
        NT_addNeighbor(state->neighbors, neigh);

        is_new_neigh = true;

        lost_messages = -1; // Initial value
    } else {
        if(NE_isDeleted(neigh)) {
            NE_setDeleted(neigh, false);
            is_new_neigh = true;
        }

        NE_setNeighborAsymTime(neigh, &asym_exp_time);
        NE_setNeighborRemovalTime(neigh, &removal_time);
        NE_setNeighborLastAnnounce(neigh, announce);

        lost_messages = (hb->seq - NE_getNeighborSEQ(neigh) - 1);
        assert(lost_messages >= 0);

        NE_setNeighborSEQ(neigh, hb->seq);
    }

    insertIntoWindow(NE_getInTraffic(neigh), &state->current_time);
    for(int i = 0; i < lost_messages; i++) {
        insertIntoWindow(NE_getMisses(neigh), &state->current_time);
    }

    if(d_type == PASSIVE_DISCOVERY) {
        // Do nothing
    } else if(d_type == HYBRID_DISCOVERY) {
        printf("has_announce: %s\n", (has_announce?"t":"f"));
    } else if(d_type == ACTIVE_DISCOVERY) {
        if(has_announce) {
            // Process Announce
            bool is_bi = false, was_bi = NE_getNeighborType(neigh, &state->current_time) == BI_NEIGH;
            bool updated_neighs = false;

            bool reply = processDiscoveryAnnounce(state->args->algorithm, state->myID, &state->current_time, state->neighbors, neigh, is_new_neigh, (unsigned char*)announce->data, announce->dataLen, &is_bi, &updated_neighs);

            updated_neigh = updated_neighs || (is_bi && !was_bi && !is_new_neigh);

            if(is_bi) {
                NE_setNeighborSymTime(neigh, &asym_exp_time); // SYM = ASYM
            }

            if(reply) {
                // TODO:
                YggEvent* ev = malloc(sizeof(YggEvent));
                YggEvent_init(ev, DISCOVERY_FRAMEWORK_PROTO_ID, REPLY_EVENT);
                YggEvent_addPayload(ev, announce, sizeof(YggMessage));
                triggerDiscoveryEvent(state, ev, true, true);
            }
        }
    } else {
        // TODO: ERROR
    }

    // Smart GC
    if( is_new_neigh && !state->gc_active ) {
        state->gc_active = true;

        struct timespec t;
        subtract_timespec(&t, NE_getNeighborAsymTime(neigh), &state->current_time);

        SetTimer(&t, state->gc_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, -1);
    }

    // Link Quality
    double my_lq = DA_computeLinkUniQuality(state->args->algorithm, NE_getMyLinkQuality(neigh), lost_messages);
    NE_setMyLinkQuality(neigh, my_lq);

    double neigh_lq = NE_getNeighLinkQuality(neigh);

    double old_lq = NE_getBiLinkQuality(neigh);
    double bi_lq = DA_computeLinkBiQuality(state->args->algorithm, my_lq, neigh_lq);
    NE_setBiLinkQuality(neigh, bi_lq);
    printf("LQ: %f %f %f\n", my_lq, neigh_lq, bi_lq);

    // Check if the new quality changed too much
    bool updated_quality = false;
    if( fabs(old_lq - bi_lq) >= state->args->lq_threshold ) {
        updated_quality = true;
    }

    // Log
    if(is_new_neigh) {
        notify_NewNeighbor(state, neigh);
        insertIntoWindow(state->stability, &state->current_time);
    } else if(updated_neigh || updated_quality) {
        printf("updated_neigh %d updated_quality %d\n", updated_neigh, updated_quality);
        notify_UpdateNeighbor(state, neigh);
        insertIntoWindow(state->stability, &state->current_time);
    }

    #ifdef DEBUG_DISCOVERY_2
    {
        if(process) {
            char neigh_id_str[UUID_STR_LEN+1];
            uuid_unparse(hb->process_id, neigh_id_str);
            neigh_id_str[UUID_STR_LEN] = '\0';

            char neigh_addr_str[30];
            wlan2asc(&announce->srcAddr, neigh_addr_str);
            neigh_addr_str[WLAN_ADDR_LEN] = '\0';

            char str[200];
            sprintf(str, "%s %s SEQ=%hu ANN=%s", neigh_addr_str, neigh_id_str, hb->seq, has_announce?"T":"F");

            char* title = NULL;
            if(is_new_neigh) {
                title = "NEW NEIGHBOR";
            } else {
                if(updated_neigh) {
                    title = "UPDATED NEIGHBOR";
                } else {
                    title = "HEARTBEAT";
                }
            }

            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, title, str);
        }
    }
    #endif
}
*/

/*

bool triggerDiscoveryEvent(discovery_framework_state* state, YggEvent* ev, bool private, bool force) {

    if(!private) {
        deliverEvent(ev);
    }

    if( imSubscribedTo(state->args->algorithm, ev->proto_origin, ev->notification_id) ) {
        list_add_item_to_tail(state->triggered_events, ev);

        scheduleAnnounce(state, force);

        return true;
    } else {
        YggEvent_freePayload(ev);
        free(ev);

        return false;
    }

}


*/


/*

unsigned long scheduleAnnounce(discovery_framework_state* state, bool force) {
    // TODO

    unsigned long t = 0L;

    DiscoveryType d_type = getDiscoveryType(state->args->algorithm);
    if(d_type != PASSIVE_DISCOVERY) {

        if( !state->announce_scheduled || force ) {
            state->announce_scheduled = true;

            struct timespec t_ = {0};

            t = lround(randomProb() * state->args->max_announce_jitter_ms);
            milli_to_timespec(&t_, t);

            SetTimer(&t_, NULL, DISCOVERY_FRAMEWORK_PROTO_ID, ANNOUNCE_TIMER);
        } else {
            // TODO
        }
    }
    // TODO: return remaining when announce is already scheduled? <--

    return t;
}

void uponAnnounceTimer(discovery_framework_state* state) {

    #ifdef DEBUG_DISCOVERY
    bool used_cache = false;
    #endif

    state->announce_scheduled = false;

    DiscoveryType d_type = getDiscoveryType(state->args->algorithm);

    bool transmit = (d_type != PASSIVE_DISCOVERY);
    bool create_announce = (d_type != PASSIVE_DISCOVERY) && (state->triggered_events->size > 0 || (d_type == HYBRID_DISCOVERY));
    if(create_announce) {
        // TODO
        // TODO : passar a queue de eventos e consumir os eventos
        unsigned char* announce = NULL;
        unsigned int announce_size = 0;
        transmit = createNewDiscoveryAnnounce(state->args->algorithm, state->myID, &state->current_time, state->neighbors, state->triggered_events, &announce, &announce_size);

        YggMessage_initBcast(&state->cached_announce, DISCOVERY_FRAMEWORK_PROTO_ID);
        YggMessage_addPayload(&state->cached_announce, (char*)announce, announce_size);
        if(announce_size > 0) {
            free(announce);
        }

        // Increment my version
        //state->my_version = (state->my_version+1 == 0) ? 1 : state->my_version+1;

        if(state->args->flush_events_upon_announce) {
            YggEvent* ev = NULL;
            while( (ev = list_remove_head(state->triggered_events)) ){
                YggEvent_freePayload(ev);
                free(ev);
            }
        } else {
            // as replies vão fazer force por isso isto não devia fazer schedule outra vez
            // scheduleAnnounce(state, false); // TODO: force?
        }

    } else {
        #ifdef DEBUG_DISCOVERY
        used_cache = true;
        #endif
    }

    if(transmit) {
        YggMessage m;
        memcpy(&m, &state->cached_announce, sizeof(YggMessage));
        // Insert heartbeat information
        piggybackHeartbeat(state, &m, false);

        // Insert into dispatcher queue
    	DF_dispatchMessage(state->dispatcher_queue, &m);

        state->stats.announces++;

        #ifdef DEBUG_DISCOVERY
        char str[100];
        sprintf(str, "New announce sent! (used cache = %s)", used_cache?"T":"F");
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "NEW ANNOUNCE", str);
        #endif
    }
    // else {
        // TODO
    }
    //
}

void DF_uponGarbageCollectorTimer(discovery_framework_state* state) {

    list* to_delete = list_init();
    list* to_remove = list_init();

    struct timespec min_exp = {0};
    bool first = true;
    void* iterator = NULL;
    NeighborEntry* current_neigh = NULL;
    while( (current_neigh = NT_nextNeighbor(state->neighbors, &iterator)) ) {

        if(!NE_isDeleted(current_neigh)) {
            struct timespec* asym_time = NE_getNeighborAsymTime(current_neigh);
            if(compare_timespec(asym_time, &state->current_time) < 0) {
                NE_setDeleted(current_neigh, true);

                list_add_item_to_tail(to_delete, current_neigh);

                struct timespec* removal_time = NE_getNeighborRemovalTime(current_neigh);
                if(compare_timespec(removal_time, &state->current_time) < 0) {
                    list_add_item_to_tail(to_remove, current_neigh);
                } else {
                    if(compare_timespec(removal_time, &min_exp) < 0 || first) {
                        copy_timespec(&min_exp, removal_time);
                        first = false;
                    }
                }
            } else {
                if(compare_timespec(asym_time, &min_exp) < 0 || first) {
                    copy_timespec(&min_exp, asym_time);
                    first = false;
                }
            }
        } else {
            struct timespec* removal_time = NE_getNeighborRemovalTime(current_neigh);
            if(compare_timespec(removal_time, &state->current_time) < 0) {
                list_add_item_to_tail(to_remove, current_neigh);
            } else {
                if(compare_timespec(removal_time, &min_exp) < 0 || first) {
                    copy_timespec(&min_exp, removal_time);
                    first = false;
                }
            }
        }
    }

    unsigned int removed = to_remove->size, lost = to_delete->size;

    while( (current_neigh = list_remove_head(to_delete)) ) {
        // TODO: NE_setNeighborRemovalTime(NeighborEntry* neigh, struct timespec* removal_time);
        notify_LostNeighbor(state, current_neigh);

        insertIntoWindow(state->stability, &state->current_time);
    }
    free(to_delete);

    while( (current_neigh = list_remove_head(to_remove)) ) {

        #ifdef DEBUG_DISCOVERY
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(NE_getNeighborID(current_neigh), id_str);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "REMOVED NEIGHBOR", id_str);
        #endif

        NT_removeNeighbor(state->neighbors, NE_getNeighborID(current_neigh));

        void* attrs = destroyNeighborEntry(current_neigh);
        // TODO: destroy attrs instead of freeing
        free(attrs);
    }
    free(to_remove);

    #ifdef DEBUG_DISCOVERY
    char next_gc_str[30];
    #endif

    // Smart GC
    if( NT_getSize(state->neighbors) > 0 ) {
        struct timespec remaining = {0};
        subtract_timespec(&remaining, &min_exp, &state->current_time);

        #ifdef DEBUG_DISCOVERY
        char str[30];
        sprintf(next_gc_str, "next GC on %s", timespec_to_string(&remaining, str, 30, 3));
        #endif

        SetTimer(&remaining, state->gc_timer_id, DISCOVERY_FRAMEWORK_PROTO_ID, -1);
    } else {
        state->gc_active = false;

        #ifdef DEBUG_DISCOVERY
        strcpy(next_gc_str, "GC OFF");
        #endif
    }

    #ifdef DEBUG_DISCOVERY
    char str[100];
    sprintf(str, "lost %d neighbors | removed %d neighbors | %s", lost, removed, next_gc_str);
    ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "GC", str);
    #endif
}


*/
