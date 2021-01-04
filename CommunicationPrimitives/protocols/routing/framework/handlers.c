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

void RF_uponDiscoveryEvent(routing_framework_state* state, YggEvent* ev) {

    void* ptr = NULL;

    switch(ev->notification_id) {
        case NEW_NEIGHBOR: {
            uuid_t id;
            ptr = YggEvent_readPayload(ev, ptr, id, sizeof(uuid_t));

            WLANAddr addr;
            ptr = YggEvent_readPayload(ev, ptr, addr.data, WLAN_ADDR_LEN);

            double rx_lq = 0.0, tx_lq = 0.0;
            ptr = YggEvent_readPayload(ev, ptr, &rx_lq, sizeof(double));
            ptr = YggEvent_readPayload(ev, ptr, &tx_lq, sizeof(double));

            double traffic = 0.0;
            ptr = YggEvent_readPayload(ev, ptr, &traffic, sizeof(double));

            byte is_bi = false;
            ptr = YggEvent_readPayload(ev, ptr, &is_bi, sizeof(byte));

            // Compute Cost
            double cost = RA_computeCost(state->args->algorithm, is_bi,rx_lq, tx_lq, NULL); // TODO: pass found time?

            assert(RN_getNeighbor(state->neighbors, id) == NULL);

            RoutingNeighborsEntry* neigh = newRoutingNeighborsEntry(id, &addr, cost, is_bi);
            RN_addNeighbor(state->neighbors, neigh);
        }
        break;
        case UPDATE_NEIGHBOR: {
            uuid_t id;
            ptr = YggEvent_readPayload(ev, ptr, id, sizeof(uuid_t));

            WLANAddr addr;
            ptr = YggEvent_readPayload(ev, ptr, addr.data, WLAN_ADDR_LEN);

            double rx_lq = 0.0, tx_lq = 0.0;
            ptr = YggEvent_readPayload(ev, ptr, &rx_lq, sizeof(double));
            ptr = YggEvent_readPayload(ev, ptr, &tx_lq, sizeof(double));

            double traffic = 0.0;
            ptr = YggEvent_readPayload(ev, ptr, &traffic, sizeof(double));

            byte is_bi = false;
            ptr = YggEvent_readPayload(ev, ptr, &is_bi, sizeof(byte));

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
            ptr = YggEvent_readPayload(ev, ptr, id, sizeof(uuid_t));

            RoutingNeighborsEntry* neigh = RN_removeNeighbor(state->neighbors, id);
            assert(neigh);

            free(neigh);
        }
        break;
    }

    RF_triggerEvent(state, RTE_NEIGHBORS_CHANGE, ev);
}

void RF_triggerEvent(routing_framework_state* state, RoutingEventType event_type, void* event_args) {

    // Compute new SEQ
    unsigned short new_seq = inc_seq(state->my_seq, state->args->ignore_zero_seq);

    // Trigger context
    YggMessage msg = {0};
    YggMessage_initBcast(&msg, ROUTING_FRAMEWORK_PROTO_ID);

    // Insert src_proto
    /*unsigned short src_proto = ROUTING_FRAMEWORK_PROTO_ID;
    int add_result = YggMessage_addPayload(&msg, (char*)&src_proto, sizeof(src_proto));
    assert(add_result != FAILED);*/

    // Insert Message Type
    byte msg_type = MSG_CONTROL_MESSAGE;
    int add_result = YggMessage_addPayload(&msg, (char*) &msg_type, sizeof(byte));
    assert(add_result != FAILED);

    bool send = RA_triggerEvent(state->args->algorithm, new_seq, event_type, event_args, state->routing_table, state->neighbors, state->myID, &state->current_time, &msg);

    if(send) {
        state->my_seq = new_seq;

        copy_timespec(&state->last_announce_time, &state->current_time);

        //pushMessageType(&msg, MSG_CONTROL_MESSAGE);

        RA_disseminateControlMessage(state->args->algorithm, &msg);
    }

}

void RF_uponNewControlMessage(routing_framework_state* state, YggMessage* message) {

    RA_rcvControlMsg(state->args->algorithm, state->routing_table, state->neighbors, state->myID, &state->current_time, message);
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
