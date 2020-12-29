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

#include "debug.h"

#include <limits.h>

#include <assert.h>

void RF_init(routing_framework_state* state) {

    getmyId(state->myID);
    memcpy(state->myAddr.data, getMyWLANAddr()->data, WLAN_ADDR_LEN);
    state->my_seq = 0;


    state->routing_table = newRoutingTable();

    state->seen_msgs = newSeenMessages();

    // Garbage Collector
	genUUID(state->gc_id);
	struct timespec _gc_interval = {0, 0};
	milli_to_timespec(&_gc_interval, state->args->gc_interval_s*1000);
	SetPeriodicTimer(&_gc_interval, state->gc_id, ROUTING_FRAMEWORK_PROTO_ID, -1);

	memset(&state->stats, 0, sizeof(routing_stats));


	//clock_gettime(CLOCK_MONOTONIC, &state->current_time); // Update current time

    genUUID(state->announce_timer_id);
    /*unsigned long announce_interval_s = getAnnounceInterval(state->args->algorithm);
    if(announce_interval_s > 0) {
        struct timespec t = {0};
        milli_to_timespec(&t, announce_interval_s*1000);
        SetPeriodicTimer(&t, state->announce_timer_id, ROUTING_FRAMEWORK_PROTO_ID, TIMER_PERIODIC_ANNOUNCE);
    }*/

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



void RF_uponNewControlMessage(routing_framework_state* state, YggMessage* message) {
    // TODO

    //processRoutingAnnounce(state->args->algorithm, state->myID, (unsigned char*)message->data, message->dataLen);
}

void RF_uponStatsRequest(routing_framework_state* state, YggRequest* req) {
    unsigned short dest = req->proto_origin;
	YggRequest_init(req, ROUTING_FRAMEWORK_PROTO_ID, dest, REPLY, REQ_ROUTING_FRAMEWORK_STATS);

	YggRequest_addPayload(req, (void*) &state->stats, sizeof(routing_stats));

	deliverReply(req);

	YggRequest_freePayload(req);
}
