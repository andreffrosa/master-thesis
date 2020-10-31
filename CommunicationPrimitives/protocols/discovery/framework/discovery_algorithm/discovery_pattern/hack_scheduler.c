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

#include "hack_scheduler_private.h"

#include <assert.h>

HackScheduler* newHackScheduler(HackSchedulerType hack_type, PiggybackType piggyback_type, bool periodic, HackReplyType reply_to_hellos, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    HackScheduler* hack_sh = malloc(sizeof(HackScheduler));

    HACK_update(hack_sh, hack_type, piggyback_type, periodic, reply_to_hellos, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);

    return hack_sh;
}

void destroyHackScheduler(HackScheduler* hack_sh) {
    assert(hack_sh);
    free(hack_sh);
}

HackScheduler* NoHACK() {
    return newHackScheduler(NO_HACK, NO_PIGGYBACK, false, NO_HACK_REPLY, false, false, false, false, false, false);
}

HackScheduler* PiggybackHACK(PiggybackType piggyback_type) {
    return newHackScheduler(PIGGYBACK_HACK, piggyback_type, false, NO_HACK_REPLY, false, false, false, false, false, false);
}

HackScheduler* PeriodicHACK(bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {
    return newHackScheduler(PERIODIC_HACK, NO_PIGGYBACK, true, NO_HACK_REPLY, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
}

HackScheduler* HybridHACK(PiggybackType piggyback_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {
    return newHackScheduler(HYBRID_HACK, piggyback_type, true, NO_HACK_REPLY, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
}

HackScheduler* ReplyHACK(HackReplyType reply_to_hellos) {
    return newHackScheduler(REPLY_HACK, NO_PIGGYBACK, false, reply_to_hellos, false, false, false, false, false, false);
}

static bool HackRepresentationInvariant(HackScheduler* hack_sh, HackSchedulerType hack_type, PiggybackType piggyback_type, bool periodic, HackReplyType reply_to_hellos, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {
    if(hack_sh == NULL)
        return false;

    bool react = react_to_new_neighbor || react_to_lost_neighbor || react_to_update_neighbor || react_to_new_2hop_neighbor || react_to_lost_2hop_neighbor || react_to_update_2hop_neighbor;

    switch( hack_type ) {
        case NO_HACK:
            return piggyback_type == NO_PIGGYBACK && !periodic && reply_to_hellos == NO_HACK_REPLY && !react;
        case PIGGYBACK_HACK:
            return piggyback_type != NO_PIGGYBACK && !periodic && reply_to_hellos == NO_HACK_REPLY && !react;
        case PERIODIC_HACK:
            return piggyback_type == NO_PIGGYBACK && periodic && reply_to_hellos == NO_HACK_REPLY;
        case HYBRID_HACK:
            return piggyback_type != NO_PIGGYBACK && periodic && reply_to_hellos == NO_HACK_REPLY;
        case REPLY_HACK:
            return piggyback_type == NO_PIGGYBACK && !periodic && reply_to_hellos != NO_HACK_REPLY; // !react ?
        default: return false;
    }
}

void HACK_update(HackScheduler* hack_sh, HackSchedulerType hack_type, PiggybackType piggyback_type, bool periodic, HackReplyType reply_to_hellos, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {
    assert( HackRepresentationInvariant(hack_sh, hack_type, piggyback_type, periodic, reply_to_hellos, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor) );

    hack_sh->type = hack_type;
    hack_sh->piggyback_type = piggyback_type;
    hack_sh->periodic = periodic;
    hack_sh->reply_to_hellos = reply_to_hellos;
    hack_sh->react_to_new_neighbor = react_to_new_neighbor;
    hack_sh->react_to_lost_neighbor = react_to_lost_neighbor;
    hack_sh->react_to_update_neighbor = react_to_update_neighbor;
    hack_sh->react_to_new_2hop_neighbor = react_to_new_2hop_neighbor;
    hack_sh->react_to_lost_2hop_neighbor = react_to_lost_2hop_neighbor;
    hack_sh->react_to_update_2hop_neighbor = react_to_update_2hop_neighbor;
}

bool HACK_isPeriodic(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->periodic;
}

PiggybackType HACK_piggybackType(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->piggyback_type;
}

HackReplyType HACK_replyToHellos(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->reply_to_hellos;
}

bool HACK_newNeighbor(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->react_to_new_neighbor;
}

bool HACK_lostNeighbor(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->react_to_lost_neighbor;
}

bool HACK_updateNeighbor(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->react_to_update_neighbor;
}

bool HACK_new2HopNeighbor(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->react_to_update_2hop_neighbor;
}

bool HACK_lost2HopNeighbor(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->react_to_update_2hop_neighbor;
}

bool HACK_update2HopNeighbor(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->react_to_update_2hop_neighbor;
}

HackSchedulerType HACK_getType(HackScheduler* hack_sh) {
    assert(hack_sh);
    return hack_sh->type;
}
