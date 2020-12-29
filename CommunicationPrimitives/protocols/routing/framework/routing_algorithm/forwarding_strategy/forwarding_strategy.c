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

#include "forwarding_strategy_private.h"

#include <assert.h>

ForwardingStrategy* newForwardingStrategy(void* args, void* vars, getNextHop get_next_hop, destroy_fs destroy) {
    assert(get_next_hop);

    ForwardingStrategy* f_strategy = malloc(sizeof(ForwardingStrategy));

    f_strategy->state.args = args;
    f_strategy->state.vars = vars;
    f_strategy->get_next_hop = get_next_hop;
    f_strategy->destroy = destroy;

    return f_strategy;
}

void destroyForwardingStrategy(ForwardingStrategy* f_strategy) {
    if(f_strategy) {
        if(f_strategy->destroy) {
            f_strategy->destroy(&f_strategy->state);
        }
        free(f_strategy);
    }
}

bool FS_getNextHop(ForwardingStrategy* f_strategy, RoutingTable* routing_table, unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr) {
    assert(f_strategy);

    return f_strategy->get_next_hop(&f_strategy->state, routing_table, destination_id, next_hop_id, next_hop_addr);
}
