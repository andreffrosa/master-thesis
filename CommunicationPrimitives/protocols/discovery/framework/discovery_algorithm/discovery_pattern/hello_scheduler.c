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

HelloScheduler* newHelloScheduler(HelloSchedulerType hello_type, PiggybackFilter* piggyback_filter, PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    HelloScheduler* hello_sh = malloc(sizeof(HelloScheduler));

    HELLO_update(hello_sh, hello_type, piggyback_filter, periodic_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);

    return hello_sh;
}

void destroyHelloScheduler(HelloScheduler* hello_sh) {
    assert(hello_sh);
    free(hello_sh);
}

HelloScheduler* NoHELLO() {
    return newHelloScheduler(NO_HELLO, NoPiggyback(), NO_PERIODIC, false, false, false, false, false, false);
}

HelloScheduler* PiggybackHELLO(PiggybackFilter* piggyback_filter) {
    return newHelloScheduler(PIGGYBACK_HELLO, piggyback_filter, NO_PERIODIC, false, false, false, false, false, false);
}

HelloScheduler* PeriodicHELLO(PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    return newHelloScheduler(PERIODIC_HELLO, NoPiggyback(), periodic_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
}

HelloScheduler* HybridHELLO(PiggybackFilter* piggyback_filter, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    return newHelloScheduler(HYBRID_HELLO, piggyback_filter, RESET_PERIODIC, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
}

static bool HelloRepresentationInvariant(HelloScheduler* hello_sh, HelloSchedulerType hello_type, PiggybackFilter* piggyback_filter, PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {
    if(hello_sh == NULL)
        return false;

    bool react = react_to_new_neighbor || react_to_lost_neighbor || react_to_update_neighbor || react_to_new_2hop_neighbor || react_to_lost_2hop_neighbor || react_to_update_2hop_neighbor;

    switch( hello_type ) {
        case NO_HELLO:
            return /*piggyback_type == NO_PIGGYBACK &&*/ periodic_type == NO_PERIODIC && !react;
        case PIGGYBACK_HELLO:
            return /*piggyback_type != NO_PIGGYBACK &&*/ periodic_type == NO_PERIODIC && !react;
        case PERIODIC_HELLO:
            return /*piggyback_type == NO_PIGGYBACK &&*/ periodic_type != NO_PERIODIC;
        case HYBRID_HELLO:
            return /*piggyback_type != NO_PIGGYBACK &&*/ periodic_type == RESET_PERIODIC;
        default: return false;
    }
}

void HELLO_update(HelloScheduler* hello_sh, HelloSchedulerType hello_type, PiggybackFilter* piggyback_filter, PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    assert( HelloRepresentationInvariant(hello_sh, hello_type, piggyback_filter, periodic_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor) );

    hello_sh->type = hello_type;
    hello_sh->piggyback_filter = piggyback_filter;
    hello_sh->periodic_type = periodic_type;
    hello_sh->react_to_new_neighbor = react_to_new_neighbor;
    hello_sh->react_to_lost_neighbor = react_to_lost_neighbor;
    hello_sh->react_to_update_neighbor = react_to_update_neighbor;
    hello_sh->react_to_new_2hop_neighbor = react_to_new_2hop_neighbor;
    hello_sh->react_to_lost_2hop_neighbor = react_to_lost_2hop_neighbor;
    hello_sh->react_to_update_2hop_neighbor = react_to_update_2hop_neighbor;
}

PeriodicType HELLO_periodicType(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->periodic_type;
}

PiggybackType HELLO_evalPiggybackFilter(HelloScheduler* hello_sh, YggMessage* msg, void* extra_args) {
    assert(hello_sh);

    PiggybackType type = evalPiggybackFilter(hello_sh->piggyback_filter, msg, extra_args);
    assert(type != UNICAST_PIGGYBACK);

    return type;
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

bool HELLO_new2HopNeighbor(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->react_to_new_2hop_neighbor;
}

bool HELLO_lost2HopNeighbor(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->react_to_lost_2hop_neighbor;
}

bool HELLO_update2HopNeighbor(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->react_to_update_2hop_neighbor;
}

HelloSchedulerType HELLO_getType(HelloScheduler* hello_sh) {
    assert(hello_sh);
    return hello_sh->type;
}
