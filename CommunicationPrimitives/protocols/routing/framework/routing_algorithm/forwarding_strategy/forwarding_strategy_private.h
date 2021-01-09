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

#ifndef _FORWARDING_STRATEGY_PRIVATE_H_
#define _FORWARDING_STRATEGY_PRIVATE_H_

#include "forwarding_strategy.h"

typedef bool (*getNextHop)(ModuleState* state, RoutingTable* routing_table,
unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, struct timespec* current_time);

typedef void (*destroy_fs)(ModuleState* state);

typedef struct ForwardingStrategy_ {
    ModuleState state;
    getNextHop get_next_hop;
    destroy_fs destroy;
} ForwardingStrategy;

ForwardingStrategy* newForwardingStrategy(void* args, void* state, getNextHop get_next_hop, destroy_fs destroy);


#endif /* _FORWARDING_STRATEGY_PRIVATE_H_ */
