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

#include "forwarding_plane.h"

#include "utility/my_time.h"
#include "utility/my_misc.h"

#include "debug.h"

#include <limits.h>

#include <assert.h>

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

    YggMessage m = {0};
    YggMessage_init(&m, toDeliver->srcAddr.data, toDeliver->Proto_id);

    // Insert src_proto
    unsigned short src_proto = ROUTING_FRAMEWORK_PROTO_ID;
    add_result = YggMessage_addPayload(&m, (char*)&src_proto, sizeof(src_proto));
    assert(add_result != FAILED);

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

    printf("XE\n");

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
        ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "SEND REQ", str);
    }
    #endif

    RF_processMessage(state, &header, &toDeliver);
}

void RF_uponNewMessage(routing_framework_state* state, YggMessage* msg) {

    // Remove Framework's header of the message
    YggMessage toDeliver;
    RoutingHeader header;
    RF_deserializeMessage(msg, &header, &toDeliver);

    // Check if the current node is the next-hop
    //bool im_next_hop = memcmp(msg->destAddr.data, state->myAddr.data, WLAN_ADDR_LEN) == 0;
    bool im_next_hop = uuid_compare(header.next_hop_id, state->myID) == 0;

    printf("im_next_hop: %s\n", (im_next_hop?"T":"F"));

    /*if(!im_next_hop) {
        WLANAddr* bcast_addr = getBroadcastAddr();
        im_next_hop = memcmp(msg->destAddr.data, bcast_addr->data, WLAN_ADDR_LEN) == 0;
        free(bcast_addr);
    }*/

    if( im_next_hop ) {

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

            printf("JJJ\n");

            RF_processMessage(state, &header, &toDeliver);
        } else {
            // Duplicate message: ignore
            printf("Duplicate message\n");
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

    bool found = RA_getNextHop(state->args->algorithm, state->routing_table, header->destination_id, next_hop_id, &next_hop_addr, &state->current_time);

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

	dispatch(&m);
	state->stats.messages_transmitted++;
}

void RF_serializeMessage(routing_framework_state* state, YggMessage* m, RoutingHeader* old_header, unsigned char* next_hop_id, unsigned char* next_hop_addr, unsigned short ttl, YggMessage* toDeliver) {

	assert(m);
    assert(next_hop_addr);
    assert(old_header);

    YggMessage_init(m, next_hop_addr, ROUTING_FRAMEWORK_PROTO_ID);

	// Initialize Broadcast Message Header
	RoutingHeader header;
    initRoutingHeader(&header, old_header->source_id, state->myID, next_hop_id, old_header->destination_id, old_header->msg_id, ttl, old_header->dest_proto);

    int msg_size = sizeof(RoutingHeader) + toDeliver->dataLen + sizeof(unsigned short) + sizeof(byte);
    assert( msg_size <= YGG_MESSAGE_PAYLOAD );

    // Insert src_proto
    unsigned short src_proto = ROUTING_FRAMEWORK_PROTO_ID;
    int add_result = YggMessage_addPayload(m, (char*)&src_proto, sizeof(src_proto));
    assert(add_result != FAILED);

    //pushMessageType(m, MSG_ROUTING_MESSAGE);

    // Insert Message Type
    byte msg_type = MSG_ROUTING_MESSAGE;
    add_result = YggMessage_addPayload(m, (char*) &msg_type, sizeof(byte));
    assert(add_result != FAILED);

	// Insert Header
	add_result = YggMessage_addPayload(m, (char*) &header, sizeof(RoutingHeader));
    assert(add_result != FAILED);

	// Insert Payload
	add_result = YggMessage_addPayload(m, (char*) toDeliver->data, toDeliver->dataLen);
    assert(add_result != FAILED);
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
