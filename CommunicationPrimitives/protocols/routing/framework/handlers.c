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

#include "utility/my_time.h"
#include "utility/my_misc.h"
#include "utility/my_math.h"
#include "utility/seq.h"

#include "debug.h"

#include <limits.h>

#include <assert.h>

void RF_init(routing_framework_state* state) {

    getmyId(state->myID);
    memcpy(state->myAddr.data, getMyWLANAddr()->data, WLAN_ADDR_LEN);
    state->my_seq = 1;

    state->routing_table = newRoutingTable();

    state->neighbors = newRoutingNeighbors();

    state->source_table = newSourceTable();

    state->seen_msgs = newSeenMessages();

    // Garbage Collector
	genUUID(state->gc_id);
	struct timespec _gc_interval = {0, 0};
	milli_to_timespec(&_gc_interval, state->args->gc_interval_s*1000);
	SetPeriodicTimer(&_gc_interval, state->gc_id, ROUTING_FRAMEWORK_PROTO_ID, -1);

	memset(&state->stats, 0, sizeof(routing_stats));

    genUUID(state->announce_timer_id);
    copy_timespec(&state->last_announce_time, &zero_timespec);
    copy_timespec(&state->next_announce_time, &zero_timespec);
    state->announce_timer_active = false;
    state->jitter_timer_active = false;
    state->send_type = NO_SEND;
    scheduleAnnounceTimer(state, true);
}

void scheduleAnnounceTimer(routing_framework_state* state, bool now) {

    unsigned int period_s = RA_getAnnouncePeriod(state->args->algorithm);

    if( period_s > 0 ) {
        if( !state->announce_timer_active ) {
            state->announce_timer_active = true;

            unsigned long jitter = (unsigned long)(randomProb()*state->args->max_jitter_ms);
            unsigned long t = now ? jitter : period_s*1000 - state->args->period_margin_ms - jitter;

            //printf("NEXT ANNOUNCE TIMER: %lu ms\n", t);

            struct timespec t_ = {0};
            milli_to_timespec(&t_, t);

            add_timespec(&state->next_announce_time, &t_, &state->current_time);

            SetTimer(&t_, state->announce_timer_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_PERIODIC_ANNOUNCE);
        }
    }
}

void RF_uponAnnounceTimer(routing_framework_state* state) {

    state->announce_timer_active = false;

    RoutingContextSendType send_type = RF_triggerEvent(state, RTE_ANNOUNCE_TIMER, NULL);

    if(send_type != NO_SEND) {
        //state->jitter_timer_active = true;

        RF_sendControlMessage(state, send_type, RTE_ANNOUNCE_TIMER, NULL, NULL);
    }

    scheduleAnnounceTimer(state, false);
}

void RF_uponSourceTimer(routing_framework_state* state, unsigned char* source_id) {

    SourceEntry* entry = ST_getEntry(state->source_table, source_id);
    //assert(entry);

    if( entry == NULL ) {
        char id_str[UUID_STR_LEN];
        uuid_unparse(source_id, id_str);
        printf("Source Entry of %s is NULL! (ERROR)\n", id_str);
        return;
    }

    if( compare_timespec(SE_getExpTime(entry), &state->current_time) > 0 ) {
        struct timespec remaining = {0};
        subtract_timespec(&remaining, SE_getExpTime(entry), &state->current_time);
        SetTimer(&remaining, source_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_SOURCE_ENTRY);
    } else {
        bool destroy = false;

        if( state->args->last_used_source_exp ) {
            RoutingTableEntry* rt_entry = RT_findEntry(state->routing_table, source_id);

            if(rt_entry) {
                struct timespec exp = {0}, delta = {0};
                struct timespec* last_used = RTE_getLastUsedTime(rt_entry);
                milli_to_timespec(&delta, SE_getPeriod(entry)*state->args->announce_misses*1000);
                add_timespec(&exp, last_used, &delta);

                if( compare_timespec(&exp, &state->current_time) <= 0 && uuid_compare(RTE_getDestinationID(rt_entry), RTE_getNextHopID(rt_entry)) != 0 ) {
                    destroy = true;
                } else {
                    SetTimer(&delta, source_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_SOURCE_ENTRY);
                }
            } else {
                destroy = true;
            }
        } else {
            destroy = true;
        }

        if(destroy) {
            #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, ADVANCED_DEBUG)
            char id_str[UUID_STR_LEN];
            uuid_unparse(source_id, id_str);

            my_logger_write(routing_logger, ROUTING_FRAMEWORK_PROTO_NAME, "SOURCE EXP", id_str);
            #endif

            ST_removeEntry(state->source_table, source_id);

            RoutingContextSendType send_type = RF_triggerEvent(state, RTE_SOURCE_EXPIRE, entry);
            if(send_type != NO_SEND) {
                RF_scheduleJitter(state, RTE_SOURCE_EXPIRE, entry, send_type);
            }

            destroySourceEntry(entry);
        }
    }
}

void RF_uponDiscoveryEvent(routing_framework_state* state, YggEvent* ev) {

    unsigned short ev_id = ev->notification_id;

    bool process = ev_id == NEW_NEIGHBOR || ev_id == UPDATE_NEIGHBOR || ev_id == LOST_NEIGHBOR;
    if(process) {
            unsigned short read = 0;
            unsigned char* ptr = ev->payload;

            unsigned short length = 0;
            memcpy(&length, ptr, sizeof(unsigned short));
            ptr += sizeof(unsigned short);
            read += sizeof(unsigned short);

            YggEvent main_ev = {0};
            YggEvent_init(&main_ev, DISCOVERY_FRAMEWORK_PROTO_ID, 0);
            YggEvent_addPayload(&main_ev, ptr, length);
            ptr += length;
            read += length;

            unsigned char* ptr2 = NULL;

            switch(ev_id) {
                case NEW_NEIGHBOR: {
                    uuid_t id;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, id, sizeof(uuid_t));

                    WLANAddr addr;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, addr.data, WLAN_ADDR_LEN);

                    double rx_lq = 0.0, tx_lq = 0.0;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, &rx_lq, sizeof(double));
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, &tx_lq, sizeof(double));

                    double traffic = 0.0;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, &traffic, sizeof(double));

                    byte is_bi = false;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, &is_bi, sizeof(byte));

                    // Compute Cost
                    double rx_cost = 0.0, tx_cost = 0.0;
                    RA_computeCost(state->args->algorithm, is_bi,rx_lq, tx_lq, &state->current_time, &rx_cost, &tx_cost);

                    assert(RN_getNeighbor(state->neighbors, id) == NULL);

                    RoutingNeighborsEntry* neigh = newRoutingNeighborsEntry(id, &addr, rx_cost, tx_cost, is_bi, &state->current_time);
                    RN_addNeighbor(state->neighbors, neigh);
                }
                break;
                case UPDATE_NEIGHBOR: {
                    uuid_t id;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, id, sizeof(uuid_t));

                    WLANAddr addr;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, addr.data, WLAN_ADDR_LEN);

                    double rx_lq = 0.0, tx_lq = 0.0;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, &rx_lq, sizeof(double));
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, &tx_lq, sizeof(double));

                    double traffic = 0.0;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, &traffic, sizeof(double));

                    byte is_bi = false;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, &is_bi, sizeof(byte));

                    RoutingNeighborsEntry* neigh = RN_getNeighbor(state->neighbors, id);
                    assert(neigh);

                    // Compute Cost
                    double rx_cost = 0.0, tx_cost = 0.0;
                    RA_computeCost(state->args->algorithm, is_bi,rx_lq, tx_lq, RNE_getFoundTime(neigh), &rx_cost, &tx_cost);

                    RNE_setRxCost(neigh, rx_cost);
                    RNE_setTxCost(neigh, tx_cost);
                    RNE_setBi(neigh, is_bi);
                }
                break;
                case LOST_NEIGHBOR: {
                    uuid_t id;
                    ptr2 = YggEvent_readPayload(&main_ev, ptr2, id, sizeof(uuid_t));

                    RoutingNeighborsEntry* neigh = RN_removeNeighbor(state->neighbors, id);
                    assert(neigh);

                    free(neigh);
                }
                break;
            }

            YggEvent_freePayload(&main_ev);

        RF_notifyCost(state);
    }

    RoutingContextSendType send_type = RF_triggerEvent(state, RTE_NEIGHBORS_CHANGE, ev);
    if(send_type != NO_SEND) {
        RF_scheduleJitter(state, RTE_NEIGHBORS_CHANGE, ev, send_type);
    }
}

bool computeNextAnnounceTimer(unsigned long max_jitter_ms, unsigned long min_interval_ms, struct timespec* current_time, struct timespec* last_timer, struct timespec* next_timer, struct timespec* t) {
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
                copy_timespec(t, &jitter);
                return true;
            }
        }
    }

    return false;
}

RoutingContextSendType RF_triggerEvent(routing_framework_state* state, RoutingEventType event_type, void* event_args) {

    RoutingContextSendType send_type = RA_triggerEvent(state->args->algorithm, event_type, event_args, state->routing_table, state->neighbors, state->source_table, state->myID, &state->current_time);

    return send_type;
}

void RF_scheduleJitter(routing_framework_state* state, RoutingEventType event_type, void* event_args, RoutingContextSendType send_type) {
    assert(event_type != RTE_ANNOUNCE_TIMER);

    if(send_type != NO_SEND) {
        if(!state->jitter_timer_active) {
            struct timespec t = {0};
            bool set = computeNextAnnounceTimer(state->args->max_jitter_ms, state->args->min_announce_interval_ms, &state->current_time, &state->last_announce_time, &state->next_announce_time, &t);

            if( set ) {
                state->jitter_timer_active = true;

                state->send_type = send_type;

                struct timespec aux1, aux2;
                milli_to_timespec(&aux1, state->args->min_announce_interval_ms);
                add_timespec(&aux1, &aux1, &state->current_time);
                subtract_timespec(&aux2, &aux1, &state->last_announce_time);

                // printf("NEXT JITTER TIMER: %lu ms | diff = %lu ms / %lu ms\n", timespec_to_milli(&t), timespec_to_milli(&aux2), state->args->min_announce_interval_ms);

                assert(timespec_to_milli(&aux2) >= state->args->min_announce_interval_ms );

                byte buffer[sizeof(RoutingContextSendType)+sizeof(RoutingEventType)];
                memcpy(buffer, &send_type, sizeof(RoutingContextSendType));
                memcpy(buffer + sizeof(RoutingContextSendType), &event_type, sizeof(RoutingEventType));

                SetTimerWithPayload(&t, NULL, ROUTING_FRAMEWORK_PROTO_ID, TIMER_SEND, (void*)buffer, sizeof(RoutingContextSendType)+sizeof(RoutingEventType));
            }
        } else {
            if( send_type == SEND_INC ) {
                state->send_type = SEND_INC;
            }
        }
    }
}

void RF_uponJitterTimer(routing_framework_state* state, RoutingContextSendType send_type, RoutingEventType event_type, void* info) {
    assert(send_type != NO_SEND && state->jitter_timer_active);

    state->jitter_timer_active = false;

    RF_sendControlMessage(state, send_type, event_type, info, NULL);
}

void RF_sendControlMessage(routing_framework_state* state, RoutingContextSendType send_type, RoutingEventType event_type, void* info, RoutingControlHeader* neigh_header) {
    assert(send_type != NO_SEND);

    state->stats.control_sent++;

    RoutingControlHeader control_header = {0};

    /*if( neigh_header ) {
        assert(send_type == SEND_NO_INC);

        memcpy(&control_header, neigh_header, sizeof(RoutingControlHeader));
    } else {*/
        // Create header
        unsigned short seq = 0;

        if( send_type == SEND_INC ) {
            // Compute new SEQ
            state->my_seq = inc_seq(state->my_seq, state->args->ignore_zero_seq);
        }
        seq = state->my_seq;

        byte period_s = RA_getAnnouncePeriod(state->args->algorithm);
        initRoutingControlHeader(&control_header, seq, period_s);
    //}

    YggMessage msg = {0};
    YggMessage_initBcast(&msg, ROUTING_FRAMEWORK_PROTO_ID);

    // Insert Message Type
    byte msg_type = MSG_CONTROL_MESSAGE;
    int add_result = YggMessage_addPayload(&msg, (char*) &msg_type, sizeof(byte));
    assert(add_result != FAILED);

    // Insert Header
    add_result = YggMessage_addPayload(&msg, (char*) &control_header, sizeof(RoutingControlHeader));
    assert(add_result != FAILED);

    RoutingContextSendType send_type2 = RA_createControlMsg(state->args->algorithm, &control_header, state->routing_table, state->neighbors, state->source_table, state->myID, &state->current_time, &msg, event_type, info);

    #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
    char str[100];
    sprintf(str, "SEQ=%hu", control_header.seq);

    my_logger_write(routing_logger, ROUTING_FRAMEWORK_PROTO_NAME, "SEND CONTROL", str);
    #endif

    copy_timespec(&state->last_announce_time, &state->current_time);

    RA_disseminateControlMessage(state->args->algorithm, state->myID, &msg, event_type, info);

    if(send_type2 != NO_SEND) {
        RF_scheduleJitter(state, RTE_CONTROL_MESSAGE_SEND, NULL, send_type2);
    }
}

void RF_uponNewControlMessage2(void* state, YggMessage* message, unsigned char* source_id, unsigned short src_proto, byte* meta_data, unsigned int meta_length) {
    RF_uponNewControlMessage((routing_framework_state*)state, message, source_id, src_proto, meta_data, meta_length);
}

void RF_uponNewControlMessage(routing_framework_state* state, YggMessage* message, unsigned char* source_id, unsigned short src_proto, byte* meta_data, unsigned int meta_length) {

    state->stats.control_received++;

    // Read header
    RoutingControlHeader header = {0};
    void* ptr = YggMessage_readPayload(message, NULL, &header, sizeof(RoutingControlHeader));

    // Read Payload
    unsigned short length = message->dataLen - sizeof(RoutingControlHeader);
    byte payload[length];
    ptr = YggMessage_readPayload(message, ptr, payload, length);

    if(uuid_compare(source_id, state->myID) != 0) {

        #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
        char id_str[UUID_STR_LEN];
        uuid_unparse(source_id, id_str);

        char str[100];
        sprintf(str, "from %s with SEQ=%hu", id_str, header.seq);

        my_logger_write(routing_logger, ROUTING_FRAMEWORK_PROTO_NAME, "RCV CONTROL", str);
        #endif

        bool process = false;
        bool new_seq = false;
        bool new_source = false;

        struct timespec exp_time = {0}, t = {0};
        milli_to_timespec(&t, header.announce_period*state->args->announce_misses*1000);
        add_timespec(&exp_time, &t, &state->current_time);

        SourceEntry* entry = ST_getEntry(state->source_table, source_id);
        if(entry == NULL) {
            entry = newSourceEntry(source_id, header.seq, header.announce_period, &exp_time);
            ST_addEntry(state->source_table, entry);

            // Set timer
            SetTimer(&t, source_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_SOURCE_ENTRY);

            process = true;
            new_seq = true;
            new_source = true;
        } else {

            if( header.seq > SE_getSEQ(entry) ) {
                //process = true;
                new_seq = true;
            } else {
                new_seq = false;
            }

            if( header.seq >= SE_getSEQ(entry) ) {
                process = true;

                SE_setSEQ(entry, header.seq);
                SE_setPeriod(entry, header.announce_period);

                // Update timer
                if( compare_timespec(SE_getExpTime(entry), &exp_time) < 0) {
                    CancelTimer(source_id, ROUTING_FRAMEWORK_PROTO_ID);
                    SetTimer(&t, source_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_SOURCE_ENTRY);
                }

                SE_setExpTime(entry, &exp_time);
            }
        }

        if(process) {

            //bool forward = false;
            RoutingContextSendType send_type = RA_processControlMsg(state->args->algorithm, state->routing_table, state->neighbors, state->source_table, entry, state->myID, &state->current_time, &header, payload, length, src_proto, meta_data, meta_length, new_seq, new_source, state->my_seq, (void*)state);

            if(send_type != NO_SEND) {

                //RF_scheduleJitter(state, RTE_REPLY, NULL, send_type);
                //state->jitter_timer_active = true;

                //RoutingControlHeader* neigh_header = forward ? &header : NULL;

                void* info = (void*[]){entry, payload, &length, &header/*, meta_data, &meta_length*/};

                RF_sendControlMessage(state, send_type, RTE_CONTROL_MESSAGE, info, NULL/* neigh_header*/);
            }
        }
    }
}

void RF_updateRoutingTable(RoutingTable* rt, list* to_update, list* to_remove, struct timespec* current_time) {

        //bool updated = RT_update(rt, to_update, to_remove);
        bool updated = false;

        if(to_update) {
            RoutingTableEntry* new_entry = NULL;
            while( (new_entry = list_remove_head(to_update)) ) {

                RoutingTableEntry* current_entry = RT_findEntry(rt, RTE_getDestinationID(new_entry));
                if(current_entry) {

                    // New Entry
                    if(uuid_compare(RTE_getNextHopID(new_entry), RTE_getNextHopID(current_entry)) != 0 || RTE_getCost(new_entry) != RTE_getCost(current_entry) || RTE_getHops(new_entry) != RTE_getHops(current_entry)) {
                            RoutingTableEntry* old_entry = RT_removeEntry(rt, RTE_getDestinationID(new_entry));
                            free(old_entry);

                            RTE_resetMessagesForwarded(new_entry);
                            //RTE_setFoundTime(current_entry, current_time);
                            RTE_setLastUsedTime(new_entry, (struct timespec*)&zero_timespec);
                            RT_addEntry(rt, new_entry);

                            updated = true;

                            #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
                            char id_str[UUID_STR_LEN];
                            uuid_unparse(RTE_getDestinationID(new_entry), id_str);

                            char str[100];
                            sprintf(str, "Replaced route to %s", id_str);
                            my_logger_write(routing_logger, ROUTING_FRAMEWORK_PROTO_NAME, "ROUTING TABLE", str);
                            #endif
                        } else {
                            free(new_entry);
                        }
                } else {
                    RT_addEntry(rt, new_entry);

                    #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
                    char id_str[UUID_STR_LEN];
                    uuid_unparse(RTE_getDestinationID(new_entry), id_str);

                    char str[100];
                    sprintf(str, "Added route to %s", id_str);
                    my_logger_write(routing_logger, ROUTING_FRAMEWORK_PROTO_NAME, "ROUTING TABLE", str);
                    #endif

                    updated = true;
                }
            }

            list_delete(to_update);
        }

        if(to_remove) {
            unsigned char* id = NULL;
            while( (id = list_remove_head(to_remove)) ) {
                RoutingTableEntry* old_entry = RT_removeEntry(rt, id);

                if(old_entry) {
                    #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
                    char id_str[UUID_STR_LEN];
                    uuid_unparse(RTE_getDestinationID(old_entry), id_str);

                    char str[100];
                    sprintf(str, "Removed route to %s", id_str);
                    my_logger_write(routing_logger, ROUTING_FRAMEWORK_PROTO_NAME, "ROUTING TABLE", str);
                    #endif

                    free(old_entry);

                    updated = true;
                }

                free(id);
            }

            free(to_remove);
        }

        if(updated) {
            #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
            char* table_str = NULL;
            RT_toString(rt, &table_str, current_time);

            char str[strlen(table_str) + 2];
            sprintf(str, "\n%s\n", table_str);
            my_logger_write(routing_table_logger, ROUTING_FRAMEWORK_PROTO_NAME, "ROUTING TABLE", str);

            free(table_str);
            #endif

            RF_notifyRoutingTable(rt);
        }

}

void RF_runGarbageCollector(routing_framework_state* state) {

    struct timespec seen_expiration = {0, 0};
    milli_to_timespec(&seen_expiration, state->args->seen_expiration_ms);

    #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, ADVANCED_DEBUG)
        unsigned int counter = SeenMessagesGC(state->seen_msgs, &state->current_time, &seen_expiration);

        char s[100];
        sprintf(s, "deleted %u messages.", counter);
        my_logger_write(routing_logger, ROUTING_FRAMEWORK_PROTO_NAME, "GC", s);
    #else
        SeenMessagesGC(state->seen_msgs, &state->current_time, &seen_expiration);
    #endif
}

/*
void RF_disseminateAnnounce(routing_framework_state* state) {
    // TODO

    unsigned char* announce_data = NULL;
    unsigned int announce_size = 0;
    unsigned short ttl = createRoutingAnnounce(state->args->algorithm, state->myID, &announce_data, &announce_size);

    // TODO: temporary
    YggMessage aux;
    YggMessage_initBcast(&aux, 0);
    YggMessage_addPayload(&aux, (char*)announce_data, announce_size);
    pushMessageType(&aux, MSG_CONTROL_MESSAGE);
    free(announce_data);

    BroadcastMessage(ROUTING_FRAMEWORK_PROTO_ID, (unsigned char*)aux.data, aux.dataLen, ttl);
}
*/

void RF_uponStatsRequest(routing_framework_state* state, YggRequest* req) {
    unsigned short dest = req->proto_origin;
	YggRequest_init(req, ROUTING_FRAMEWORK_PROTO_ID, dest, REPLY, REQ_ROUTING_FRAMEWORK_STATS);

	YggRequest_addPayload(req, (void*) &state->stats, sizeof(routing_stats));

	deliverReply(req);

	YggRequest_freePayload(req);
}

void RF_notifyCost(routing_framework_state* state) {
    YggEvent ev = {0};
    YggEvent_init(&ev, ROUTING_FRAMEWORK_PROTO_ID, EV_ROUTING_NEIGHS);

    byte amount = RN_getSize(state->neighbors);
    YggEvent_addPayload(&ev, &amount, sizeof(byte));

    void* iterator = NULL;
    RoutingNeighborsEntry* current_neigh = NULL;
    while( (current_neigh = RN_nextNeigh(state->neighbors, &iterator)) ) {

        YggEvent_addPayload(&ev, RNE_getID(current_neigh), sizeof(uuid_t));

        YggEvent_addPayload(&ev, RNE_getAddr(current_neigh)->data, WLAN_ADDR_LEN);

        double rx_cost = RNE_getRxCost(current_neigh);
        YggEvent_addPayload(&ev, &rx_cost, sizeof(double));

        double tx_cost = RNE_getTxCost(current_neigh);
        YggEvent_addPayload(&ev, &tx_cost, sizeof(double));

        byte is_bi = RNE_isBi(current_neigh);
        YggEvent_addPayload(&ev, &is_bi, sizeof(byte));

        YggEvent_addPayload(&ev, RNE_getFoundTime(current_neigh), sizeof(struct timespec));
    }

    deliverEvent(&ev);
    YggEvent_freePayload(&ev);
}

void RF_notifyRoutingTable(RoutingTable* rt) {
    YggEvent ev = {0};
    YggEvent_init(&ev, ROUTING_FRAMEWORK_PROTO_ID, EV_ROUTING_TABLE);

    byte amount = RT_size(rt);
    YggEvent_addPayload(&ev, &amount, sizeof(byte));

    void* iterator = NULL;
    RoutingTableEntry* re = NULL;
    while( (re = RT_nextRoute(rt, &iterator)) ) {

        YggEvent_addPayload(&ev, RTE_getDestinationID(re), sizeof(uuid_t));

        YggEvent_addPayload(&ev, RTE_getNextHopID(re), sizeof(uuid_t));

        YggEvent_addPayload(&ev, RTE_getNextHopAddr(re)->data, WLAN_ADDR_LEN);

        double c = RTE_getCost(re);
        YggEvent_addPayload(&ev, &c, sizeof(double));

        byte b = RTE_getHops(re);
        YggEvent_addPayload(&ev, &b, sizeof(byte));
    }

    deliverEvent(&ev);
    YggEvent_freePayload(&ev);
}
