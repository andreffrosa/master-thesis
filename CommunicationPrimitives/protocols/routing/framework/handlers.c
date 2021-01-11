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
    state->my_seq = 0;

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
    state->announce_timer_active = false;
    scheduleAnnounceTimer(state, true);
}

void scheduleAnnounceTimer(routing_framework_state* state, bool now) {

    unsigned int period_s = RA_getAnnouncePeriod(state->args->algorithm);

    if( period_s > 0 ) {
        if( !state->announce_timer_active ) {
            state->announce_timer_active = true;

            unsigned long jitter = (unsigned long)(randomProb()*state->args->max_jitter_ms);

            unsigned long t = now ? jitter : period_s*1000 - state->args->period_margin_ms - jitter;

            printf("NEXT TIMER: %lu ms\n", t);

            struct timespec t_ = {0};
            milli_to_timespec(&t_, t);
            SetTimer(&t_, state->announce_timer_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_PERIODIC_ANNOUNCE);
        }
    }
}

void RF_uponAnnounceTimer(routing_framework_state* state) {

    state->announce_timer_active = false;
    // copy_timespec(&state->last_announce_time, &state->current_time);

    RF_triggerEvent(state, RTE_ANNOUNCE_TIMER, NULL);

    printf("UPON TIMER\n");

    scheduleAnnounceTimer(state, false);
}

void RF_uponSourceTimer(routing_framework_state* state, unsigned char* source_id) {

    SourceEntry* entry = ST_getEntry(state->source_table, source_id);
    assert(entry);

    if( compare_timespec(SE_getExpTime(entry), &state->current_time) > 0 ) {
        struct timespec remaining = {0};
        subtract_timespec(&remaining, SE_getExpTime(entry), &state->current_time);
        SetTimer(&remaining, source_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_SOURCE_ENTRY);
    } else {
        #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, ADVANCED_DEBUG)
        char id_str[UUID_STR_LEN];
        uuid_unparse(source_id, id_str);

        ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "SOURCE EXP", id_str);
        #endif

        ST_removeEntry(state->source_table, source_id);

        RF_triggerEvent(state, RTE_SOURCE_EXPIRE, entry);

        destroySourceEntry(entry);
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
                    double cost = RA_computeCost(state->args->algorithm, is_bi,rx_lq, tx_lq, NULL); // TODO: pass found time?

                    assert(RN_getNeighbor(state->neighbors, id) == NULL);

                    RoutingNeighborsEntry* neigh = newRoutingNeighborsEntry(id, &addr, cost, is_bi);
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

                    // Compute Cost
                    double cost = RA_computeCost(state->args->algorithm, is_bi,rx_lq, tx_lq, NULL); // TODO: pass found time?

                    RoutingNeighborsEntry* neigh = RN_getNeighbor(state->neighbors, id);
                    assert(neigh);

                    RNE_setCost(neigh, cost);
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
    }

    RF_triggerEvent(state, RTE_NEIGHBORS_CHANGE, ev);
}

void RF_triggerEvent(routing_framework_state* state, RoutingEventType event_type, void* event_args) {

    RoutingContextSendType send = RA_triggerEvent(state->args->algorithm, event_type, event_args, state->routing_table, state->neighbors, state->source_table, state->myID, &state->current_time);

    unsigned short seq = 0;

    if( send == SEND_INC ) {
        // Compute new SEQ
        state->my_seq = inc_seq(state->my_seq, state->args->ignore_zero_seq);
    }
    seq = state->my_seq;

    if( send == SEND_INC || send == SEND_NO_INC  ) {
        // Trigger context
        YggMessage msg = {0};
        YggMessage_initBcast(&msg, ROUTING_FRAMEWORK_PROTO_ID);

        // Insert Message Type
        byte msg_type = MSG_CONTROL_MESSAGE;
        int add_result = YggMessage_addPayload(&msg, (char*) &msg_type, sizeof(byte));
        assert(add_result != FAILED);

        // Insert Header
        RoutingControlHeader header = {0};
        byte period_s = RA_getAnnouncePeriod(state->args->algorithm);
        initRoutingControlHeader(&header, state->myID, seq, period_s);

        add_result = YggMessage_addPayload(&msg, (char*) &header, sizeof(RoutingControlHeader));
        assert(add_result != FAILED);


        RA_createControlMsg(state->args->algorithm, &header, state->routing_table, state->neighbors, state->source_table, state->myID, &state->current_time, &msg);

        #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
        char id_str[UUID_STR_LEN];
        uuid_unparse(header.source_id, id_str);

        char str[100];
        sprintf(str, "seq %hu\n", header.seq);

        ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "SEND CONTROL", str);
        #endif

        copy_timespec(&state->last_announce_time, &state->current_time);

        RA_disseminateControlMessage(state->args->algorithm, &msg);
    }
}

void RF_uponNewControlMessage(routing_framework_state* state, YggMessage* message) {

    // Read header
    RoutingControlHeader header = {0};
    void* ptr = YggMessage_readPayload(message, NULL, &header, sizeof(RoutingControlHeader));

    // Read Payload
    unsigned short length = message->dataLen - sizeof(RoutingControlHeader);
    byte payload[length];
    ptr = YggMessage_readPayload(message, ptr, payload, length);

    //
    if(uuid_compare(header.source_id, state->myID) != 0) {

        bool process = false;

        struct timespec exp_time = {0}, t = {0};
        milli_to_timespec(&t, header.announce_period*state->args->announce_misses*1000);
        add_timespec(&exp_time, &t, &state->current_time);

        SourceEntry* entry = ST_getEntry(state->source_table, header.source_id);
        if(entry == NULL) {
            entry = newSourceEntry(header.source_id, header.seq, &exp_time, NULL);
            ST_addEntry(state->source_table, entry);

            // Set timer
            SetTimer(&t, header.source_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_SOURCE_ENTRY);

            process = true;
        } else {
            if( header.seq > SE_getSEQ(entry) ) {
                process = true;
            }

            if( header.seq >= SE_getSEQ(entry) ) {
                SE_setSEQ(entry, header.seq);

                // Update timer
                if( compare_timespec(SE_getExpTime(entry), &exp_time) < 0) {
                    CancelTimer(header.source_id, ROUTING_FRAMEWORK_PROTO_ID);
                    SetTimer(&t, header.source_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_SOURCE_ENTRY);
                }

                SE_setExpTime(entry, &exp_time);
            }
        }

        if(process) {
            #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, ADVANCED_DEBUG)
            char id_str[UUID_STR_LEN];
            uuid_unparse(header.source_id, id_str);

            char str[100];
            sprintf(str, "from %s with seq %hu\n", id_str, header.seq);

            ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "RCV CONTROL", str);
            #endif

            RA_processControlMsg(state->args->algorithm, state->routing_table, state->neighbors, state->source_table, entry, state->myID, &state->current_time, &header, payload, length);
        }
    }
}

void RF_updateRoutingTable(RoutingTable* rt, list* to_update, list* to_remove, struct timespec* current_time) {

    #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
        bool updated = RT_update(rt, to_update, to_remove);

        if(updated) {
            char* table_str = NULL;
            RT_toString(rt, &table_str, current_time);

            char str[strlen(table_str) + 2];
            sprintf(str, "\n%s\n", table_str);
            ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "ROUTING TABLE", str);

            free(table_str);
        }
    #else
        RT_update(rt, to_update, to_remove);
    #endif
}


void RF_runGarbageCollector(routing_framework_state* state) {

    struct timespec seen_expiration = {0, 0};
    milli_to_timespec(&seen_expiration, state->args->seen_expiration_ms);

    #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, ADVANCED_DEBUG)
        unsigned int counter = SeenMessagesGC(state->seen_msgs, &state->current_time, &seen_expiration);

        char s[100];
        sprintf(s, "deleted %u messages.", counter);
        ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "GC", s);
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
