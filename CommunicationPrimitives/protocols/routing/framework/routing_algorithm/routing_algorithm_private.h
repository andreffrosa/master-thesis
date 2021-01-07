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

#ifndef _ROUTING_ALGORITHM_PRIVATE_H_
#define _ROUTING_ALGORITHM_PRIVATE_H_

#include "routing_algorithm.h"

#include "routing_context/routing_context_private.h"
#include "forwarding_strategy/forwarding_strategy_private.h"
#include "cost_metric/cost_metric_private.h"
#include "announce_period/announce_period_private.h"
#include "dissemination_strategy/dissemination_strategy_private.h"

typedef struct RoutingAlgorithm_ {
    RoutingContext* r_context;
    ForwardingStrategy* f_strategy;
    CostMetric* cost_metric;
    AnnouncePeriod* a_period;
    DisseminationStrategy* d_strategy;
} RoutingAlgorithm;

#endif /* _ROUTING_ALGORITHM_PRIVATE_H_ */