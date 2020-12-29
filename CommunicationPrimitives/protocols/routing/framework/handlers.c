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

void RF_DeliverMessage(routing_framework_state* state, RoutingHeader* header, YggMessage* toDeliver) {

    #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
    {
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(header->msg_id, id_str);

        char str[UUID_STR_LEN+4];
        sprintf(str, "[%s]", id_str);
        ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "DELIVER", str);
    }
    #endif

    int add_result = 0;

    YggMessage m;
    YggMessage_initBcast(&m, toDeliver->Proto_id);

    // Insert payload size
    unsigned short payload_size = toDeliver->dataLen;
    add_result = YggMessage_addPayload(&m, (char*)&payload_size, sizeof(payload_size));
    assert(add_result != FAILED);

    // Insert payload
    add_result = YggMessage_addPayload(&m, (char*)toDeliver->data, payload_size);
    assert(add_result != FAILED);

    // Insert Routing Source Node's ID
    add_result = YggMessage_addPayload(&m, (char*)header->source_id, sizeof(uuid_t));
    assert(add_result != FAILED);

    // TODO: more metadata

    deliver(&m);
	state->stats.messages_delivered++;
}

void RF_uponRouteRequest(routing_framework_state* state, YggRequest* req) {
    state->stats.messages_requested++;

    // Deserialize request
    void* ptr = NULL;
    uuid_t destination_id;
    ptr = YggRequest_readPayload(req, ptr, destination_id, sizeof(uuid_t));

    unsigned short ttl = 0;
    ptr = YggRequest_readPayload(req, ptr, &ttl, sizeof(unsigned short));

    unsigned int len = req->length - sizeof(uuid_t) - sizeof(unsigned short);
    unsigned char data[len];
    ptr = YggRequest_readPayload(req, ptr, data, len);

    YggMessage toDeliver;
    YggMessage_init(&toDeliver, state->myAddr.data, req->proto_origin);
	YggMessage_addPayload(&toDeliver, (char*)data, len);

    uuid_t msg_id;
    genUUID(msg_id);

    if(ttl != USHRT_MAX) { // Different than infinity
        ttl++;
    }

    RoutingHeader header;
    initRoutingHeader(&header, state->myID, state->myID, state->myID, destination_id, msg_id, ttl, req->proto_origin);

    #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
    {
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(destination_id, id_str);

        char msg_id_str[UUID_STR_LEN+1];
        msg_id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(msg_id, msg_id_str);

        char msg[len+1];
        memcpy(msg, data, len);
        msg[len] = '\0';

        char str[300];
        sprintf(str, "[%s] to %s : %s", msg_id_str, id_str, msg);
        ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "ROUTE REQ", str);
    }
    #endif

    RF_processMessage(state, &header, &toDeliver);
}

void RF_uponNewMessage(routing_framework_state* state, YggMessage* msg) {

    // Check if the current node is the next-hop
    bool im_next_hop = memcmp(msg->destAddr.data, getMyWLANAddr()->data, WLAN_ADDR_LEN) == 0;

    if(!im_next_hop) {
        WLANAddr* bcast_addr = getBroadcastAddr();
        im_next_hop = memcmp(msg->destAddr.data, bcast_addr->data, WLAN_ADDR_LEN) == 0;
        free(bcast_addr);
    }

    if( im_next_hop ) {

        // Remove Framework's header of the message
        YggMessage toDeliver;
        RoutingHeader header;
        RF_deserializeMessage(msg, &header, &toDeliver);

        bool send_ack = false; // TODO:
        if(send_ack) {
            // TODO
        }

        // Check if is not a duplicate
        if( !SeenMessagesContains(state->seen_msgs, header.msg_id) ) {
            state->stats.messages_received++;

            #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, ADVANCED_DEBUG)
            {
                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(header.msg_id, id_str);

                char str[150];
                sprintf(str, "[%s]", id_str);
                ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "RECEIVED", str);
            }
            #endif

            RF_processMessage(state, &header, &toDeliver);
        } else {
            // Duplicate message: ignore
        }
    } else {
        // Message is not meant for this node: ignore
    }
}

void RF_processMessage(routing_framework_state* state, RoutingHeader* header, YggMessage* toDeliver) {

    SeenMessagesAdd(state->seen_msgs, header->msg_id, &state->current_time);

    // Check if message is destined to the local address
    if(uuid_compare(header->destination_id, state->myID) == 0) {
        RF_DeliverMessage(state, header, toDeliver);
    } else {
        unsigned short ttl = header->ttl;

        // Decrement TTL
        if(ttl != USHRT_MAX) { // Different than infinity
            ttl--;
        }

        if(ttl > 0) {
            RF_ForwardMessage(state, header, ttl, toDeliver);
        }
    }
}

void RF_ForwardMessage(routing_framework_state* state, RoutingHeader* header, unsigned short ttl, YggMessage* toDeliver) {

    // Find next hop
    WLANAddr next_hop_addr;
    uuid_t next_hop_id;

    bool found = RA_getNextHop(state->args->algorithm, state->routing_table, header->destination_id, next_hop_id, &next_hop_addr);

    if(found) {
        // Send
        RF_SendMessage(state, header, next_hop_id, next_hop_addr.data, ttl, toDeliver);

        #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, ADVANCED_DEBUG)
        {
            char id_str[UUID_STR_LEN+1];
            id_str[UUID_STR_LEN] = '\0';
            uuid_unparse(header->msg_id, id_str);

            char next_hop_id_str[UUID_STR_LEN+1];
            next_hop_id_str[UUID_STR_LEN] = '\0';
            uuid_unparse(next_hop_id, next_hop_id_str);

            char str[100];
            sprintf(str, "[%s] to %s", id_str, next_hop_id_str);
            ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "FORWARD", str);
        }
        #endif
    } else {

        // TODO: RREQ

        //#ifdef DEBUG_ROUTING
        {
            char id_str[UUID_STR_LEN+1];
            id_str[UUID_STR_LEN] = '\0';
            uuid_unparse(header->msg_id, id_str);
            char str[UUID_STR_LEN+4];
            sprintf(str, "[%s]", id_str);
            ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "ROUTE NOT KNOWN", str);
        }
        //#endif
    }
}

void RF_SendMessage(routing_framework_state* state, RoutingHeader* old_header, unsigned char* next_hop_id, unsigned char* next_hop_addr, unsigned short ttl, YggMessage* toDeliver) {
    YggMessage m;

    RF_serializeMessage(state, &m, old_header, next_hop_id, next_hop_addr, ttl, toDeliver);

    pushMessageType(&m, MSG_ROUTING_MESSAGE);

	dispatch(&m);
	state->stats.messages_transmitted++;
}

void RF_serializeMessage(routing_framework_state* state, YggMessage* m, RoutingHeader* old_header, unsigned char* next_hop_id, unsigned char* next_hop_addr, unsigned short ttl, YggMessage* toDeliver) {

	assert(state != NULL);
	assert(m != NULL);
    assert(old_header);

    YggMessage_init(m, next_hop_addr, ROUTING_FRAMEWORK_PROTO_ID);

	// Initialize Broadcast Message Header
	RoutingHeader header;
    initRoutingHeader(&header, old_header->source_id, state->myID, next_hop_id, old_header->destination_id, old_header->msg_id, ttl, old_header->dest_proto);

    int msg_size = sizeof(RoutingHeader) + toDeliver->dataLen;
    assert( msg_size <= YGG_MESSAGE_PAYLOAD );

	// Insert Header
	YggMessage_addPayload(m, (char*) &header, sizeof(RoutingHeader));

	// Insert Payload
	YggMessage_addPayload(m, (char*) toDeliver->data, toDeliver->dataLen);
}

void RF_deserializeMessage(YggMessage* m, RoutingHeader* header, YggMessage* toDeliver) {

	void* ptr = NULL;
	ptr = YggMessage_readPayload(m, ptr, header, sizeof(RoutingHeader));

	unsigned short payload_size = m->dataLen - sizeof(RoutingHeader);

	YggMessage_init(toDeliver, m->destAddr.data, header->dest_proto);
	memcpy(toDeliver->srcAddr.data, m->srcAddr.data, WLAN_ADDR_LEN);
	toDeliver->dataLen = payload_size;
	ptr = YggMessage_readPayload(m, ptr, toDeliver->data, payload_size);
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
