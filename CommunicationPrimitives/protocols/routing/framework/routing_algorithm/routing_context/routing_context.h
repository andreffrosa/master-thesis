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

void destroyRoutingContext(RoutingContext* context);

void RCtx_init(RoutingContext* context, proto_def* protocol_definition, unsigned char* myID, RoutingTable* r_table, struct timespec* current_time);

///////////////////////////////////////////////////////////////////

RoutingContext* StaticRoutingContext();

#endif /* _ROUTING_CONTEXT_H_ */
