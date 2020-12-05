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

#include "utility/my_time.h"
#include "utility/my_math.h"
#include "utility/my_misc.h"

#include <assert.h>

void* broadcast_framework_main_loop(main_loop_args* args);

static bool processTimer(broadcast_framework_state* f_state, YggTimer* timer);
static bool processMessage(broadcast_framework_state* f_state, YggMessage* message);
static bool processRequest(broadcast_framework_state* f_state, YggRequest* request);
static bool processEvent(broadcast_framework_state* f_state, YggEvent* event);

proto_def* broadcast_framework_init(void* arg) {

	broadcast_framework_state* f_state = malloc(sizeof(broadcast_framework_state));
	f_state->args = (broadcast_framework_args*)arg;

	proto_def* framework = create_protocol_definition(BROADCAST_FRAMEWORK_PROTO_ID, BROADCAST_FRAMEWORK_PROTO_NAME, f_state, NULL);
	proto_def_add_protocol_main_loop(framework, &broadcast_framework_main_loop);

	// Initialize f_state variables
	init(f_state);

	BA_initRetransmissionContext(f_state->args->algorithm, framework, f_state->myID);

	return framework;
}

void* broadcast_framework_main_loop(main_loop_args* args) {

	broadcast_framework_state* f_state = args->state;

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
			processEvent(f_state, &elem.data.event);
            processed = false; // Events should always be processed by both the framework and the context(?)
			break;
		default:
			; // On purpose
			char s[100];
			sprintf(s, "Got weird queue elem, type = %u", elem.type);
			ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "MAIN LOOP", s);
			exit(-1);
		}

        // this eent is meant to be processed by the Retransmission Context
        if(!processed) {
            BA_processEvent(f_state->args->algorithm, &elem, f_state->myID);
        }

		// Release memory of elem payload
		free_elem_payload(&elem);
	}

	return NULL;
}

static bool processTimer(broadcast_framework_state* f_state, YggTimer* timer) {

    // Message Timeout
    if( timer->timer_type == TIMER_BROADCAST_MESSAGE_TIMEOUT ) {
        uponTimeout(f_state, timer);
        return true;
    }
    // Garbage Collector
    else if( uuid_compare(timer->id, f_state->gc_id) == 0 ) {
        runGarbageCollector(f_state);
        return true;
    }

    // This timer is meant to be processed by the Retransmission Context
    return false;
}

static bool processMessage(broadcast_framework_state* f_state, YggMessage* message) {

    BcastMessageType type = (BcastMessageType)popMessageType(message);

    // Broadcast Message
    if(type == MSG_BROADCAST_MESSAGE) {
        uponNewMessage(f_state, message);
        return true;
    }

    // This message is meant to be processed by the Retransmission Context
    return false;
}

static bool processRequest(broadcast_framework_state* f_state, YggRequest* request) {

    if( request->proto_dest == BROADCAST_FRAMEWORK_PROTO_ID ) {
        if( request->request == REQUEST ) {

            // Broadcast Message Request
            if( request->request_type == REQ_BROADCAST_MESSAGE ) {
                uponBroadcastRequest(f_state, request);
                return true;
            }

            // Get Stats Request
            else if ( request->request_type == REQ_BROADCAST_FRAMEWORK_STATS ) {
                uponStatsRequest(f_state, request);
                return true;
            }
        }

        // This request/reply is (probably) meant to be processed by the Retransmission Context
        return false;
    }

    else {
        char s[100];
        if(request->request == REPLY)
            sprintf(s, "Received reply from protocol %d meant for protocol %d", request->proto_origin, request->proto_dest);
        else
            sprintf(s, "Received request from protocol %d meant for protocol %d", request->proto_origin, request->proto_dest);

        ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "ERROR", s);

        return true;
    }
}

static bool processEvent(broadcast_framework_state* f_state, YggEvent* event) {

    if( event->proto_dest == BROADCAST_FRAMEWORK_PROTO_ID ) {
        // Currently the framework does not process any event

        // This event is meant to be processed by the Retransmission Context
        return false;
    }

    else {
        char s[100];
        sprintf(s, "Received event from protocol %d meant for protocol %d", event->proto_origin, event->proto_dest);
        ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "ERROR", s);

        return true;
    }
}

void BroadcastMessage(unsigned short protocol_id, unsigned short ttl, byte* data, unsigned short size) {
    YggRequest framework_bcast_req;
    YggRequest_init(&framework_bcast_req, protocol_id, BROADCAST_FRAMEWORK_PROTO_ID, REQUEST, REQ_BROADCAST_MESSAGE);

    YggRequest_addPayload(&framework_bcast_req, &ttl, sizeof(ttl));
    YggRequest_addPayload(&framework_bcast_req, data, size);

    deliverRequest(&framework_bcast_req);

    YggRequest_freePayload(&framework_bcast_req);
}
