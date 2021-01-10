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

static double ETX_f(ModuleState* m_state, bool is_bi, double rx_lq, double tx_lq, struct timespec* found_time) {
    return 1.0/(rx_lq * tx_lq);
}

CostMetric* ETXMetric() {
    return newCostMetric(
        NULL,
        NULL,
        &ETX_f,
        NULL
    );
}