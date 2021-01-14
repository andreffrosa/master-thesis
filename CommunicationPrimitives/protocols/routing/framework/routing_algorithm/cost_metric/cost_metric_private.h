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

#ifndef _COST_METRIC_PRIVATE_H_
#define _COST_METRIC_PRIVATE_H_

#include "../common.h"

typedef void (*cost_function)(ModuleState*, bool, double, double, struct timespec*, double* rx_cost, double* tx_cost);

typedef void (*cm_destroy)(ModuleState*);

typedef struct CostMetric_ {
    ModuleState state;

    cost_function f;

    cm_destroy destroy;
} CostMetric;

CostMetric* newCostMetric(void* args, void* vars, cost_function f, cm_destroy destroy);

#endif /*_COST_METRIC_PRIVATE_H_*/
