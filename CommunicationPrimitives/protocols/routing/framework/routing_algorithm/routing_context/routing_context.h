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

#ifndef _ROUTING_CONTEXT_H_
#define _ROUTING_CONTEXT_H_

#include "../common.h"

typedef struct _RoutingContext RoutingContext;

typedef enum {
    NO_SEND,
    SEND_NO_INC,
    SEND_INC
} RoutingContextSendType;

void destroyRoutingContext(RoutingContext* context);

void RCtx_init(RoutingContext* context, proto_def* protocol_definition, unsigned char* myID, RoutingTable* r_table, struct timespec* current_time);

RoutingContextSendType RCtx_triggerEvent(RoutingContext* context, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time);

void RCtx_createMsg(RoutingContext* context, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info);

RoutingContextSendType RCtx_processMsg(RoutingContext* context, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, unsigned short src_proto, byte* meta_data, unsigned int meta_length);

const char* RCtx_getID(RoutingContext* context);

///////////////////////////////////////////////////////////////////

RoutingContext* StaticRoutingContext();

RoutingContext* OLSRRoutingContext();

RoutingContext* AODVRoutingContext();

RoutingContext* ZoneRoutingContext(RoutingContext* proactive_ctx, RoutingContext* reactive_ctx);


#endif /* _ROUTING_CONTEXT_H_ */
