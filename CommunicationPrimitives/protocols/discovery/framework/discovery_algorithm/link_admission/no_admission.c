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

static bool eval(ModuleState* m_state, NeighborEntry* neigh, struct timespec* current_time) {
    return true;
}

LinkAdmission* NoAdmission() {
    return newLinkAdmissionPolicy(
        NULL,
        NULL,
        &eval,
        NULL
    );
}
