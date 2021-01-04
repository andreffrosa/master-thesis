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

#include "link_admission_private.h"

typedef struct HysteresisArgs_ {
    double hyst_reject;
    double hyst_accept;
} HysteresisArgs;

static bool eval(ModuleState* m_state, NeighborEntry* neigh, struct timespec* current_time) {

    HysteresisArgs* args = (HysteresisArgs*)m_state->args;

    double rx_lq = NE_getRxLinkQuality(neigh);

    bool pending = NE_isPending(neigh);

    bool accept = pending ? (rx_lq >= args->hyst_accept) : (rx_lq >= args->hyst_reject);

    return accept;
}

LinkAdmission* HysteresisAdmission(double hyst_reject, double hyst_accept) {

    HysteresisArgs* args = malloc(sizeof(HysteresisArgs));
    args->hyst_reject = hyst_reject;
    args->hyst_accept = hyst_accept;

    return newLinkAdmissionPolicy(
        args,
        NULL,
        &eval,
        NULL
    );
}
