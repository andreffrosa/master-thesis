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
    add_result = YggMessage_addPayload(&m, (char*)&src_proto, sizeof(unsigned short));
    assert(add_result != FAILED);

    // Insert payload size
    unsigned short payload_size = toDeliver->dataLen;
    add_result = YggMessage_addPayload(&m, (char*)&payload_size, sizeof(unsigned short));
    assert(add_result != FAILED);

    // Insert payload
    add_result = YggMessage_addPayload(&m, (char*)toDeliver->data, payload_size);
    assert(add_result != FAILED);

    // Insert Routing Source Node's ID
    //add_result = YggMessage_addPayload(&m, (char*)header->source_id, sizeof(uuid_t));
    //assert(add_result != FAILED);

    add_result = YggMessage_addPayload(&m, (char*)header, sizeof(RoutingHeader));
    assert(add_result != FAILED);

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

    byte hop_delivery = false;
    ptr = YggRequest_readPayload(req, ptr, &hop_delivery, sizeof(byte));

    unsigned int len = req->length - sizeof(uuid_t) - sizeof(unsigned short) - sizeof(byte);
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
    initRoutingHeader(&header, state->myID, state->myID, state->myID, destination_id, msg_id, ttl, req->proto_origin, hop_delivery, 0);

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

    RF_processMessage(state, &header, NULL, true, &toDeliver);
}

void RF_uponNewMessage(routing_framework_state* state, YggMessage* msg) {

    // Remove Framework's header of the message
    YggMessage toDeliver;
    RoutingHeader header;
    byte meta_data[1000] = {0};
    RF_deserializeMessage(msg, &header, meta_data, &toDeliver);

    // Check if the current node is the next-hop
    //bool im_next_hop = memcmp(msg->destAddr.data, state->myAddr.data, WLAN_ADDR_LEN) == 0;
    bool im_next_hop = uuid_compare(header.next_hop_id, state->myID) == 0;

    //printf("im_next_hop: %s\n", (im_next_hop?"T":"F"));

    /*if(!im_next_hop) {
        WLANAddr* bcast_addr = getBroadcastAddr();
        im_next_hop = memcmp(msg->destAddr.data, bcast_addr->data, WLAN_ADDR_LEN) == 0;
        free(bcast_addr);
    }*/

    if( im_next_hop || header.hop_delivery ) {

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


            RF_processMessage(state, &header, meta_data, false, &toDeliver);
        } else {
            // Duplicate message: ignore or send ack
            printf("Duplicate message\n");
        }
    } else {
        // Message is not meant for this node: ignore
    }
}

void RF_processMessage(routing_framework_state* state, RoutingHeader* header, byte* prev_meta_data, bool first, YggMessage* toDeliver) {

    SeenMessagesAdd(state->seen_msgs, header->msg_id, &state->current_time);

    bool deliver = uuid_compare(header->destination_id, state->myID) == 0 || header->hop_delivery;
    bool forward = uuid_compare(header->destination_id, state->myID) != 0;

    // Check if message is destined to the local address
    if(deliver) {

        if( toDeliver->Proto_id == ROUTING_FRAMEWORK_PROTO_ID ) {
            if( uuid_compare(header->source_id, state->myID) != 0 ) {
                void* ptr = NULL;

                byte aux = 0;
                ptr = YggMessage_readPayload(toDeliver, ptr, &aux, sizeof(byte));
                RoutingMessageType type = aux;

                if(type == MSG_CONTROL_MESSAGE) {

                    YggMessage m;
                    YggMessage_init(&m, toDeliver->srcAddr.data, toDeliver->Proto_id);

                    unsigned short length = toDeliver->dataLen - sizeof(byte);
                    YggMessage_addPayload(&m, ptr, length);

                    void* info = (void*[]){header, prev_meta_data, &first};

                    printf("RECEIVED CONTROL MSG \n");


                    RF_uponNewControlMessage(state, &m, header->source_id, ROUTING_FRAMEWORK_PROTO_ID, info, sizeof(RoutingHeader));
                } else {
                    assert(false);
                }
            }

        } else{
            RF_DeliverMessage(state, header, toDeliver);
        }

    }

    if(forward) {
        unsigned short ttl = header->ttl;

        // Decrement TTL
        if(ttl != USHRT_MAX) { // Different than infinity
            ttl--;
        }

        bool im_next_hop = uuid_compare(header->next_hop_id, state->myID) == 0;

        if(ttl > 0 && im_next_hop) {
            RF_ForwardMessage(state, header, prev_meta_data, ttl, first, toDeliver);
        }
    }
}

void RF_ForwardMessage(routing_framework_state* state, RoutingHeader* header, byte* prev_meta_data, unsigned short ttl, bool first, YggMessage* toDeliver) {

    // Find next hop
    WLANAddr next_hop_addr;
    uuid_t next_hop_id;

    unsigned short meta_data_length = 0;
    byte* meta_data = NULL;

    bool found = RA_getNextHop(state->args->algorithm, state->routing_table, state->source_table, state->neighbors, state->myID, header->destination_id, next_hop_id, &next_hop_addr, &meta_data, &meta_data_length, prev_meta_data, header->meta_data_length, first, &state->current_time);

    if(found) {
        // Send
        RF_SendMessage(state, header, next_hop_id, next_hop_addr.data, meta_data, meta_data_length, ttl, toDeliver);

        if(meta_data) {
            free(meta_data);
        }

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

        bool print = true;

        // RREQ ?
        //if( uuid_compare(header->source_id, state->myID) == 0 ) {
            RoutingContextSendType send_type = RF_triggerEvent(state, RTE_ROUTE_NOT_FOUND, header->destination_id);
            if(send_type != NO_SEND) {
                state->jitter_timer_active = true;
                RF_sendControlMessage(state, send_type, RTE_ROUTE_NOT_FOUND, header, NULL);
                print = false;

                /// TODO: put in buffer
            }
        //}

        if( print ) {
            #if DEBUG_INCLUDE_GT(ROUTING_DEBUG_LEVEL, SIMPLE_DEBUG)
            char id_str[UUID_STR_LEN];
            uuid_unparse(header->msg_id, id_str);

            char str[UUID_STR_LEN+4];
            sprintf(str, "[%s]", id_str);

            ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "ROUTE NOT KNOWN", str);
            #endif
        }
    }
}

void RF_SendMessage(routing_framework_state* state, RoutingHeader* old_header, unsigned char* next_hop_id, unsigned char* next_hop_addr, byte* new_meta_data, unsigned short new_meta_data_length, unsigned short ttl, YggMessage* toDeliver) {

    YggMessage toForward = {0};

    if(uuid_compare(old_header->source_id, state->myID) != 0 && toDeliver->Proto_id == ROUTING_FRAMEWORK_PROTO_ID) {
        //if( uuid_compare(old_header->source_id, state->myID) != 0 ) {

            void* ptr = NULL;

            byte aux = 0;
            ptr = YggMessage_readPayload(toDeliver, ptr, &aux, sizeof(byte));
            RoutingMessageType type = aux;

            if(type == MSG_CONTROL_MESSAGE) {
                RoutingControlHeader old_routing_control_header;
                ptr = YggMessage_readPayload(toDeliver, ptr, &old_routing_control_header, sizeof(RoutingControlHeader));

                // Read Payload
                unsigned short length = toDeliver->dataLen - sizeof(byte) - sizeof(RoutingControlHeader);
                byte payload[length];
                ptr = YggMessage_readPayload(toDeliver, ptr, payload, length);

                SourceEntry* entry = ST_getEntry(state->source_table, old_header->source_id);
                assert(entry);
                assert(length > 0);
                void* info = (void*[]){entry, payload, &length, old_header};

                //YggMessage msg = {0};
                YggMessage_initBcast(&toForward, ROUTING_FRAMEWORK_PROTO_ID);

                // Insert Message Type
                byte msg_type = MSG_CONTROL_MESSAGE;
                int add_result = YggMessage_addPayload(&toForward, (char*) &msg_type, sizeof(byte));
                assert(add_result != FAILED);

                // Insert Header
                add_result = YggMessage_addPayload(&toForward, (char*) &old_routing_control_header, sizeof(RoutingControlHeader));
                assert(add_result != FAILED);

                RA_createControlMsg(state->args->algorithm, &old_routing_control_header, state->routing_table, state->neighbors, state->source_table, state->myID, &state->current_time, &toForward, RTE_CONTROL_MESSAGE, info);

            } else {
                assert(false);
            }
        //}
    } else {
        memcpy(&toForward, toDeliver, sizeof(YggMessage));
    }

    YggMessage m;

    RF_serializeMessage(state, &m, old_header, next_hop_id, next_hop_addr, new_meta_data, new_meta_data_length, ttl, &toForward);

	dispatch(&m);
	state->stats.messages_transmitted++;
}

void RF_serializeMessage(routing_framework_state* state, YggMessage* m, RoutingHeader* old_header, unsigned char* next_hop_id, unsigned char* next_hop_addr, byte* new_meta_data, unsigned short new_meta_data_length, unsigned short ttl, YggMessage* toDeliver) {

	assert(m);
    assert(next_hop_addr);
    assert(old_header);

    YggMessage_init(m, next_hop_addr, ROUTING_FRAMEWORK_PROTO_ID);

	// Initialize Broadcast Message Header
	RoutingHeader header;
    initRoutingHeader(&header, old_header->source_id, state->myID, next_hop_id, old_header->destination_id, old_header->msg_id, ttl, old_header->dest_proto, old_header->hop_delivery, new_meta_data_length);

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

    // Insert Meta-Data
    if(header.meta_data_length > 0) {
        add_result = YggMessage_addPayload(m, (char*) new_meta_data, header.meta_data_length);
        assert(add_result != FAILED);
    }

	// Insert Payload
	add_result = YggMessage_addPayload(m, (char*) toDeliver->data, toDeliver->dataLen);
    assert(add_result != FAILED);
}

void RF_deserializeMessage(YggMessage* m, RoutingHeader* header, byte* meta_data, YggMessage* toDeliver) {

	void* ptr = NULL;
	ptr = YggMessage_readPayload(m, ptr, header, sizeof(RoutingHeader));

    if(header->meta_data_length > 0) {
        ptr = YggMessage_readPayload(m, ptr, meta_data, header->meta_data_length);
    }

	unsigned short payload_size = m->dataLen - sizeof(RoutingHeader) - header->meta_data_length;

	YggMessage_init(toDeliver, m->destAddr.data, header->dest_proto);
	memcpy(toDeliver->srcAddr.data, m->srcAddr.data, WLAN_ADDR_LEN);
	toDeliver->dataLen = payload_size;
	ptr = YggMessage_readPayload(m, ptr, toDeliver->data, payload_size);
}
