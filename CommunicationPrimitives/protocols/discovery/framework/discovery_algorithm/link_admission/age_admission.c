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

    unsigned int min_hellos = *((unsigned int*)m_state->args);

    struct timespec* found_time = NE_getNeighborFoundTime(neigh);

    struct timespec elapsed_t = {0};
    subtract_timespec(&elapsed_t, current_time, found_time);
    unsigned long elapsed = timespec_to_milli(&elapsed_t);

    unsigned long hello_period = NE_getNeighborHelloPeriod(neigh);

    unsigned int n_hellos = ((double)elapsed) / hello_period;

    return n_hellos >= min_hellos;
}

LinkAdmission* AgeAdmission(unsigned int min_hellos) {

    unsigned int* min_hellos_arg = malloc(sizeof(unsigned int));
    *min_hellos_arg = min_hellos;

    return newLinkAdmissionPolicy(
        min_hellos_arg,
        NULL,
        &eval,
        NULL
    );
}
