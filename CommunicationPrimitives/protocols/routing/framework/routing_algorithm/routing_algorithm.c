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

#include <stdlib.h>
#include <assert.h>

#include "routing_algorithm_private.h"

RoutingAlgorithm* newRoutingAlgorithm(RoutingContext* r_context, ForwardingStrategy* f_strategy, CostMetric* cost_metric) {
    assert(r_context && f_strategy && cost_metric);

	RoutingAlgorithm* alg = (RoutingAlgorithm*)malloc(sizeof(RoutingAlgorithm));

    alg->r_context = r_context;
    alg->f_strategy = f_strategy;
    alg->cost_metric = cost_metric;

	return alg;
}

void destroyRoutingAlgorithm(RoutingAlgorithm* alg) {
    if(alg != NULL) {
        // TODO
        free(alg);
    }
}

bool RA_getNextHop(RoutingAlgorithm* alg, RoutingTable* routing_table, unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr) {
    assert(alg);

    return FS_getNextHop(alg->f_strategy, routing_table, destination_id, next_hop_id, next_hop_addr);
}

void RA_init(RoutingAlgorithm* alg, proto_def* protocol_definition, unsigned char* myID, RoutingTable* r_table, struct timespec* current_time) {
    assert(alg);

    RCtx_init(alg->r_context, protocol_definition, myID, r_table, current_time);
}

double RA_computeCost(RoutingAlgorithm* alg, bool is_bi, double rx_lq, double tx_lq, struct timespec* found_time) {
    assert(alg);

    return CM_compute(alg->cost_metric, is_bi, rx_lq, tx_lq, found_time);
}
