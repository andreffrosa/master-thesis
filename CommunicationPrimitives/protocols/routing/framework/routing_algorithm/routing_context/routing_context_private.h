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

typedef RoutingContextSendType (*rc_triggerEvent)(ModuleState* m_state, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time);

typedef void (*rc_createMsg)(ModuleState* m_state, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info);

typedef RoutingContextSendType (*rc_processMsg)(ModuleState* m_state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, byte* meta_data, unsigned int meta_length, bool* forward);

typedef void (*rc_destroy)(ModuleState* m_state);

typedef struct _RoutingContext {
    ModuleState state;

	rc_init init;

    rc_triggerEvent trigger_event;

    rc_createMsg create_msg;
    rc_processMsg process_msg;

    rc_destroy destroy;
} RoutingContext;

RoutingContext* newRoutingContext(void* args, void* vars, rc_init init, rc_triggerEvent trigger_event, rc_createMsg create_msg, rc_processMsg rcv_msg, rc_destroy destroy);

#endif /* _ROUTING_CONTEXT_PRIVATE_H_ */
