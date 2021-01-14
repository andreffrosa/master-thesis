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

static void Hops_f(ModuleState* m_state, bool is_bi, double rx_lq, double tx_lq, struct timespec* found_time, double* rx_cost, double* tx_cost) {
    *rx_cost = 1.0;
    *tx_cost = 1.0;
}

CostMetric* HopsMetric() {
    return newCostMetric(
        NULL,
        NULL,
        &Hops_f,
        NULL
    );
}
