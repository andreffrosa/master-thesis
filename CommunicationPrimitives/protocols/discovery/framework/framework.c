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

#include <assert.h>

void* discovery_framework_main_loop(main_loop_args* args);

static bool processTimer(discovery_framework_state* f_state, YggTimer* timer);
static bool processMessage(discovery_framework_state* f_state, YggMessage* message);
static bool processRequest(discovery_framework_state* f_state, YggRequest* request);
static bool processEvent(discovery_framework_state* f_state, YggEvent* event);

proto_def* discovery_framework_init(void* arg) {

	discovery_framework_state* f_state = malloc(sizeof(discovery_framework_state));
	f_state->args = (discovery_framework_args*)arg;

	proto_def* framework = create_protocol_definition(DISCOVERY_FRAMEWORK_PROTO_ID, DISCOVERY_FRAMEWORK_PROTO_NAME, f_state, NULL);
	proto_def_add_protocol_main_loop(framework, &discovery_framework_main_loop);
    proto_def_add_produced_events(framework, (DiscoveryEventType)DISCOVERY_EVENT_COUNT);

    f_state->dispatcher_queue = interceptProtocolQueue(PROTO_DISPATCH, DISCOVERY_FRAMEWORK_PROTO_ID);

    clock_gettime(CLOCK_MONOTONIC, &f_state->current_time); // Update current time

	// Initialize f_state variables
	DF_init(f_state);

	return framework;
}

void* discovery_framework_main_loop(main_loop_args* args) {

	discovery_framework_state* f_state = args->state;

	queue_t_elem elem;
	while (1) {
		// Retrieve an event from the queue
		queue_pop(args->inBox, &elem);

        bool processed = false;

        // Update current time
		clock_gettime(CLOCK_MONOTONIC, &f_state->current_time);

		switch (elem.type) {
    		case YGG_TIMER:
                ; YggTimer* timer = &elem.data.timer;
                if(timer->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID) {
                    processed = processTimer(f_state, timer);
                } else {
                    queue_push(f_state->dispatcher_queue, &elem);
                    processed = true;
                }
    			break;

    		case YGG_MESSAGE:
                ; YggMessage* msg = &elem.data.msg;
                processed = processMessage(f_state, msg);
                break;
    		case YGG_REQUEST:
                ; YggRequest* req = &elem.data.request;
                if(req->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID) {
                    processed = processRequest(f_state, &elem.data.request);
                } else {
                    queue_push(f_state->dispatcher_queue, &elem);
                    processed = true;
                }
    			break;
    		case YGG_EVENT:
                ; YggEvent* event = &elem.data.event;
                if(event->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID) {
                    processEvent(f_state, &elem.data.event);
                    processed = false; // Events should always be processed by both the framework and the context(?)
                } else {
                    queue_push(f_state->dispatcher_queue, &elem);
                    processed = true;
                }
    			break;
    		default:
    			; // On purpose
    			char s[100];
    			sprintf(s, "Got weird queue elem, type = %u", elem.type);
    			ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", s);
    			exit(-1);
		}

        if(!processed) {
            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "Event not processed");
        }

		// Release memory of elem payload
		free_elem_payload(&elem);
	}

	return NULL;
}

static bool processTimer(discovery_framework_state* f_state, YggTimer* timer) {

    // HELLO Timer
    if( timer->timer_type == HELLO_TIMER ) {
        DF_uponHelloTimer(f_state);
        return true;
    }
    // HACK Timer
    else if( timer->timer_type == HACK_TIMER ) {
        DF_uponHackTimer(f_state);
        return true;
    }
    // Reply Timer
    else if( timer->timer_type == REPLY_TIMER ) {
        DF_uponReplyTimer(f_state, timer->payload, timer->length);
        return true;
    }
    // Neighbor Change Timer
    else if( timer->timer_type == NEIGHBOR_CHANGE_TIMER ) {
        DF_uponNeighborChangeTimer(f_state);
        return true;
    }
    // Neighbor Timer
    else if( timer->timer_type == NEIGHBOR_TIMER ) {
        NeighborEntry* neigh = NT_getNeighbor(f_state->neighbors, timer->id);
        assert(neigh);
        DF_uponNeighborTimer(f_state, neigh);
        return true;
    }
    // Discovery Environment Timer
    else if( timer->timer_type == DISCOVERY_ENVIRONMENT_TIMER ) {
        DF_uponDiscoveryEnvironmentTimer(f_state);
        return true;
    }

    return false;
}

static bool processMessage(discovery_framework_state* f_state, YggMessage* msg) {
    bool processed = false;

    // UpStream message
    if(msg->Proto_id == DISCOVERY_FRAMEWORK_PROTO_ID) {
        // Check if there are piggybacked discovery messages
        unsigned short piggyback_size = 0;
        memcpy(&piggyback_size, msg->data + sizeof(unsigned short), sizeof(piggyback_size));

        byte buffer[sizeof(piggyback_size) + piggyback_size];
        popPayload(msg, (char*)buffer, sizeof(piggyback_size) + piggyback_size);

        assert( piggyback_size == *((unsigned char*)buffer) );

        // Has discovery message
        if( piggyback_size > 0 ) {
            bool piggybacked = msg->Proto_id != DISCOVERY_FRAMEWORK_PROTO_ID;

            DF_processMessage(f_state, buffer + sizeof(piggyback_size), piggyback_size, piggybacked, &msg->srcAddr);
        }

        // Destination is other protocol
        if(msg->Proto_id != DISCOVERY_FRAMEWORK_PROTO_ID) {
            filterAndDeliver(msg);
        }

        processed = true;
    }
    // DownStream msg
    else {
        // Piggyback an hello
        DF_piggybackDiscovery(f_state, msg);

        // Insert into dispatcher queue
        DF_dispatchMessage(f_state->dispatcher_queue, msg);
        processed = true;
    }

    return processed;
}

static bool processRequest(discovery_framework_state* f_state, YggRequest* request) {

    if( request->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID ) {
        if( request->request == REQUEST ) {

            // Get Stats Request
            if( request->request_type == REQ_DISCOVERY_FRAMEWORK_STATS ) {
                    DF_uponStatsRequest(f_state, request);
                    return true;
                }
            }

            return false;
        }
        else {
            char s[100];
            sprintf(s, "Received %s from protocol %d meant for protocol %d", (request->request == REPLY ? "relpy" : "request") , request->proto_origin, request->proto_dest);

            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ERROR", s);

            return true;
        }
}

static bool processEvent(discovery_framework_state* f_state, YggEvent* event) {

    if( event->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID ) {
        return false;
    }
    else {
        char s[100];
        sprintf(s, "Received event from protocol %d meant for protocol %d", event->proto_origin, event->proto_dest);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ERROR", s);

        return false;
    }
}
