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

#ifndef _COST_METRIC_H_
#define _COST_METRIC_H_

#include "../common.h"

typedef struct CostMetric_ CostMetric;

void destroyCostMetric(CostMetric* cm);

void CM_compute(CostMetric* cm, bool is_bi, double rx_lq, double tx_lq, struct timespec* found_time, double* rx_cost, double* tx_cost);

///////////////////////////////////////////////////

CostMetric* HopsMetric();

CostMetric* ETXMetric();


#endif /*_COST_METRIC_H_*/