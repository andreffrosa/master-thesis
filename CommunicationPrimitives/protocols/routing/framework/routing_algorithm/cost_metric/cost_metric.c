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

#include "cost_metric_private.h"

#include <assert.h>

CostMetric* newCostMetric(void* args, void* vars, cost_function f, cm_destroy destroy) {
    assert(f);

    CostMetric* cm = malloc(sizeof(CostMetric));

    cm->state.args = args;
    cm->state.vars = vars;
    cm->f = f;
    cm->destroy = destroy;

    return cm;
}

void destroyCostMetric(CostMetric* cm) {
    if(cm) {
        if(cm->destroy) {
            cm->destroy(&cm->state);
        }
        free(cm);
    }
}

double CM_compute(CostMetric* cm, bool is_bi, double rx_lq, double tx_lq, struct timespec* found_time) {
    assert(cm);
    return cm->f(&cm->state, is_bi, rx_lq, tx_lq, found_time);
}
