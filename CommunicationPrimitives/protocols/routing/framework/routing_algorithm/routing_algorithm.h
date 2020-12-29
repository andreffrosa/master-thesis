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

#ifndef _ROUTING_ALGORITHM_H_
#define _ROUTING_ALGORITHM_H_

#include <stdarg.h>

#include "common.h"

#include "routing_context/routing_context.h"
#include "forwarding_strategy/forwarding_strategy.h"
#include "cost_metric/cost_metric.h"

typedef struct RoutingAlgorithm_ RoutingAlgorithm;

RoutingAlgorithm* newRoutingAlgorithm(RoutingContext* context, ForwardingStrategy* f_strategy, CostMetric* cost_metric);

void destroyRoutingAlgorithm(RoutingAlgorithm* algorithm);

bool RA_getNextHop(RoutingAlgorithm* alg, RoutingTable* routing_table, unsigned char* destination_hop_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr);

void RA_init(RoutingAlgorithm* alg, proto_def* protocol_definition, unsigned char* myID, RoutingTable* r_table, struct timespec* current_time);

double RA_computeCost(RoutingAlgorithm* alg, bool is_bi, double rx_lq, double tx_lq, struct timespec* found_time);

#endif /* _ROUTING_ALGORITHM_H_ */
