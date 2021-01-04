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

#include "framework.h"
#include "handlers.h"
#include "forwarding_plane.h"

#include "utility/my_time.h"
#include "utility/my_math.h"
#include "utility/my_misc.h"

#include "protocols/broadcast/framework/framework.h"

#include <assert.h>

void* routing_framework_main_loop(main_loop_args* args);

static bool processTimer(routing_framework_state* f_state, YggTimer* timer);
static bool processMessage(routing_framework_state* f_state, YggMessage* message);
static bool processRequest(routing_framework_state* f_state, YggRequest* request);
static bool processEvent(routing_framework_state* f_state, YggEvent* event);

proto_def* routing_framework_init(void* arg) {

	routing_framework_state* f_state = malloc(sizeof(routing_framework_state));
	f_state->args = (routing_framework_args*) arg;

	proto_def* framework = create_protocol_definition(ROUTING_FRAMEWORK_PROTO_ID, ROUTING_FRAMEWORK_PROTO_NAME, f_state, NULL);
	proto_def_add_protocol_main_loop(framework, &routing_framework_main_loop);

    proto_def_add_consumed_event(framework, DISCOVERY_FRAMEWORK_PROTO_ID, NEW_NEIGHBOR);
    proto_def_add_consumed_event(framework, DISCOVERY_FRAMEWORK_PROTO_ID, UPDATE_NEIGHBOR);
    proto_def_add_consumed_event(framework, DISCOVERY_FRAMEWORK_PROTO_ID, LOST_NEIGHBOR);
    proto_def_add_consumed_event(framework, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBORHOOD);
    proto_def_add_consumed_event(framework, DISCOVERY_FRAMEWORK_PROTO_ID, GENERIC_DISCOVERY_EVENT);
    proto_def_add_consumed_event(framework, DISCOVERY_FRAMEWORK_PROTO_ID, DISCOVERY_ENVIRONMENT_UPDATE);

    // Update current time
    clock_gettime(CLOCK_MONOTONIC, &f_state->current_time);

	// Initialize f_state variables
	RF_init(f_state);

    RA_init(f_state->args->algorithm, framework, f_state->myID, f_state->routing_table, &f_state->current_time);

	return framework;
}

void* routing_framework_main_loop(main_loop_args* args) {

	routing_framework_state* f_state = args->state;

	queue_t_elem elem;
	while (1) {
		// Retrieve an event from the queue
		queue_pop(args->inBox, &elem);

        // Update current time
		clock_gettime(CLOCK_MONOTONIC, &f_state->current_time);

        bool processed = false;

		switch (elem.type) {
    		case YGG_TIMER:
    			processed = processTimer(f_state, &elem.data.timer);
    			break;
    		case YGG_MESSAGE:
                processed = processMessage(f_state, &elem.data.msg);
                break;
    		case YGG_REQUEST:
    			processed = processRequest(f_state, &elem.data.request);
    			break;
    		case YGG_EVENT:
    			processed = processEvent(f_state, &elem.data.event);
    			break;
    		default: {
    			char s[100];
    			sprintf(s, "Got weird queue elem, type = %u", elem.type);
    			ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "MAIN LOOP", s);
    			exit(-1);
            }
		}

        if(!processed) {
            ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "event not processed");
        }

		// Release memory of elem payload
		free_elem_payload(&elem);
	}

	return NULL;
}

static bool processTimer(routing_framework_state* f_state, YggTimer* timer) {

    printf("QQQ\n");

    if(timer->timer_type == TIMER_PERIODIC_ANNOUNCE) {
        if( uuid_compare(timer->id, f_state->announce_timer_id) == 0 ) {
            RF_uponAnnounceTimer(f_state);
            return true;
        }
    } else {
        // Garbage Collector
        if( uuid_compare(timer->id, f_state->gc_id) == 0 ) {
            RF_runGarbageCollector(f_state);
            return true;
        }
    }

    return false;
}

static bool processMessage(routing_framework_state* f_state, YggMessage* message) {

    unsigned short src_proto = 0;
    byte aux = 0;
    RoutingMessageType type = 0;

    void* ptr = NULL;
    ptr = YggMessage_readPayload(message, ptr, &src_proto, sizeof(unsigned short));

    printf("SRC PROTO: %hu\n", src_proto);

    YggMessage msg;
    YggMessage_init(&msg, message->srcAddr.data, message->Proto_id);

    if( src_proto == BROADCAST_FRAMEWORK_PROTO_ID ) {
        unsigned short payload_size = 0;
        ptr = YggMessage_readPayload(message, ptr, &payload_size, sizeof(unsigned short));

        // ptr = YggMessage_readPayload(message, ptr, &src_proto, sizeof(unsigned short));

        ptr = YggMessage_readPayload(message, ptr, &aux, sizeof(byte));
        type = aux;

        if(type == MSG_CONTROL_MESSAGE) {
            YggMessage_addPayload(&msg, ptr, payload_size - sizeof(byte));

            // TODO: copy and deliver metadata

            RF_uponNewControlMessage(f_state, &msg);
            return true;
        }
    } else if( src_proto == ROUTING_FRAMEWORK_PROTO_ID ) {
        ptr = YggMessage_readPayload(message, ptr, &aux, sizeof(byte));
        type = aux;

        if(type == MSG_ROUTING_MESSAGE) {
            YggMessage_addPayload(&msg, ptr, message->dataLen - sizeof(unsigned short) - sizeof(byte));

            RF_uponNewMessage(f_state, &msg);
            return true;
        }
    }

    return false;
}

static bool processRequest(routing_framework_state* f_state, YggRequest* request) {

    if( request->proto_dest == ROUTING_FRAMEWORK_PROTO_ID ) {
        if( request->request == REQUEST ) {

            // Route Message Request
            if( request->request_type == REQ_ROUTE_MESSAGE ) {
                RF_uponRouteRequest(f_state, request);
                return true;
            }

            // Get Stats Request
            else if ( request->request_type == REQ_ROUTING_FRAMEWORK_STATS ) {
                RF_uponStatsRequest(f_state, request);
                return true;
            }
        }

        return false;
    }

    else {
        char s[100];
        if(request->request == REPLY)
            sprintf(s, "Received reply from protocol %d meant for protocol %d", request->proto_origin, request->proto_dest);
        else
            sprintf(s, "Received request from protocol %d meant for protocol %d", request->proto_origin, request->proto_dest);

        ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "ERROR", s);

        return true;
    }
}

static bool processEvent(routing_framework_state* f_state, YggEvent* event) {

    if( event->proto_dest == ROUTING_FRAMEWORK_PROTO_ID ) {

        if(event->proto_origin == DISCOVERY_FRAMEWORK_PROTO_ID) {
            RF_uponDiscoveryEvent(f_state, event);

            return true;
        }

        return false;
    }

    else {
        char s[100];
        sprintf(s, "Received event from protocol %d meant for protocol %d", event->proto_origin, event->proto_dest);
        ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "ERROR", s);

        return true;
    }
}

void RouteMessage(unsigned char* destination_id, short protocol_id, unsigned short ttl, unsigned char* data, unsigned int size) {
    YggRequest framework_route_req;
	YggRequest_init(&framework_route_req, protocol_id, ROUTING_FRAMEWORK_PROTO_ID, REQUEST, REQ_ROUTE_MESSAGE);

    YggRequest_addPayload(&framework_route_req, destination_id, sizeof(uuid_t));
    YggRequest_addPayload(&framework_route_req, &ttl, sizeof(unsigned short));
    YggRequest_addPayload(&framework_route_req, data, size);

    deliverRequest(&framework_route_req);
	YggRequest_freePayload(&framework_route_req);
}
