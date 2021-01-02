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
 * (C) 2019
 *********************************************************/

#ifndef _ROUTING_CONTEXT_PRIVATE_H_
#define _ROUTING_CONTEXT_PRIVATE_H_

#include "routing_context.h"

typedef void (*rc_init)(ModuleState* m_state, proto_def* protocol_definition, unsigned char* myID, RoutingTable* r_table, struct timespec* current_time);

typedef bool (*rc_triggerEvent)(ModuleState* m_state, unsigned short seq, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, unsigned char* myID, YggMessage* msg);

typedef void (*rc_rcvMsg)(ModuleState* m_state, RoutingTable* routing_table, RoutingNeighbors* neighbors, YggMessage* msg);

typedef void (*rc_destroy)(ModuleState* m_state);

typedef struct _RoutingContext {
    ModuleState state;

	rc_init init;

    rc_triggerEvent trigger_event;

    rc_rcvMsg rcv_msg;

    rc_destroy destroy;
} RoutingContext;

RoutingContext* newRoutingContext(void* args, void* vars, rc_init init, rc_triggerEvent trigger_event, rc_rcvMsg rcv_msg, rc_destroy destroy);

#endif /* _ROUTING_CONTEXT_PRIVATE_H_ */
