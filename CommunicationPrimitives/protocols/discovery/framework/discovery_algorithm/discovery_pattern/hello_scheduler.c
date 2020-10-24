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

#include "hello_scheduler_private.h"

#include <assert.h>

HelloScheduler* newHelloScheduler(HelloSchedulerType hello_type, PiggybackType piggyback_type, bool periodic, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor) {

    HelloScheduler* hello_sh = malloc(sizeof(HelloScheduler));

    HELLO_update(hello_sh, hello_type, piggyback_type, periodic, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor);

    return hello_sh;
}

void destroyHelloScheduler(HelloScheduler* hello_sh) {
    assert(hello_sh);
    free(hello_sh);
}

HelloScheduler* NoHELLO() {
    return newHelloScheduler(NO_HELLO, NO_PIGGYBACK, false, false, false, false);
}

HelloScheduler* PiggybackHELLO(PiggybackType piggyback_type) {
    return newHelloScheduler(PIGGYBACK_HELLO, piggyback_type, false, false, false, false);
}

HelloScheduler* PeriodicHELLO(bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor) {
    return newHelloScheduler(PERIODIC_HELLO, NO_PIGGYBACK, true, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor);
}

HelloScheduler* HybridHELLO(PiggybackType piggyback_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor) {
    return newHelloScheduler(HYBRID_HELLO, piggyback_type, true, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor);
}

static bool HelloRepresentationInvariant(HelloScheduler* hello_sh, HelloSchedulerType hello_type, PiggybackType piggyback_type, bool periodic, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor) {
    if(hello_sh == NULL)
        return false;

    switch( hello_type ) {
        case NO_HELLO:
            return piggyback_type == NO_PIGGYBACK && !periodic && !react_to_new_neighbor && !react_to_lost_neighbor && !react_to_update_neighbor;
        case PIGGYBACK_HELLO:
            return piggyback_type != NO_PIGGYBACK && !periodic && !react_to_new_neighbor && !react_to_lost_neighbor && !react_to_update_neighbor;
        case PERIODIC_HELLO:
            return piggyback_type == NO_PIGGYBACK && periodic;
        case HYBRID_HELLO:
            return piggyback_type != NO_PIGGYBACK && periodic;
        default: return false;
    }
}

void HELLO_update(HelloScheduler* hello_sh, HelloSchedulerType hello_type, PiggybackType piggyback_type, bool periodic, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor) {
    assert( HelloRepresentationInvariant(hello_sh, hello_type, piggyback_type, periodic, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor) );

    hello_sh->type = hello_type;
    hello_sh->piggyback_type = piggyback_type;
    hello_sh->periodic = periodic;
    hello_sh->react_to_new_neighbor = react_to_new_neighbor;
    hello_sh->react_to_lost_neighbor = react_to_lost_neighbor;
    hello_sh->react_to_update_neighbor = react_to_update_neighbor;
}

bool HELLO_isPeriodic(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->periodic;
}

PiggybackType HELLO_piggybackType(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->piggyback_type;
}

bool HELLO_newNeighbor(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->react_to_new_neighbor;
}

bool HELLO_lostNeighbor(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->react_to_lost_neighbor;
}

bool HELLO_updateNeighbor(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->react_to_update_neighbor;
}

HelloSchedulerType HELLO_getType(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->type;
}
