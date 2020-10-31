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

#include "discovery_pattern_private.h"

#include <assert.h>

void destroyDiscoveryPattern(DiscoveryPattern* dp) {
    if(dp) {
        destroyHelloScheduler(dp->hello_sh);
        destroyHackScheduler(dp->hack_sh);
        free(dp);
    }
}

DiscoveryPattern* newDiscoveryPattern(HelloScheduler* hello_sh, HackScheduler* hack_sh) {
    assert(hello_sh && hack_sh);
    assert( !(hello_sh->type == NO_HELLO) || (hack_sh->type == NO_HACK) );
    assert( !(hello_sh->type == PIGGYBACK_HELLO) || (hack_sh->type == NO_HACK) );
    assert( !(hack_sh->type == REPLY_HACK) || (hello_sh->type == PERIODIC_HELLO || hello_sh->type == HYBRID_HELLO) );

    DiscoveryPattern* dp = malloc(sizeof(DiscoveryPattern));

    dp->hello_sh = hello_sh;
    dp->hack_sh = hack_sh;

    return dp;
}

DiscoveryPattern* NoDiscovery() {
    return newDiscoveryPattern(NoHELLO(), NoHACK());
}

DiscoveryPattern* PassiveDiscovery(PiggybackType hello_piggyback_type, PiggybackType hack_piggyback_type) {
    HackScheduler* hack_sh = (hack_piggyback_type == NO_PIGGYBACK ? NoHACK() : PiggybackHACK(hack_piggyback_type));

    return newDiscoveryPattern(PiggybackHELLO(hello_piggyback_type), hack_sh);
}

DiscoveryPattern* HybridHelloDiscovery(PiggybackType hello_piggyback_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    return newDiscoveryPattern(
        HybridHELLO(hello_piggyback_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor),
        NoHACK()
    );
}

DiscoveryPattern* PeriodicHelloDiscovery(bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    return newDiscoveryPattern(
        PeriodicHELLO(react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor),
        NoHACK()
    );
}

DiscoveryPattern* PeriodicJointDiscovery(bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    return newDiscoveryPattern(
        PeriodicHELLO(react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor),
        PiggybackHACK(PIGGYBACK_ON_DISCOVERY_TRAFFIC) // param?
    );
}

DiscoveryPattern* PeriodicDisjointDiscovery(bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    // TODO: disjoint parameters?

    return newDiscoveryPattern(
        PeriodicHELLO(react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor),
        PeriodicHACK(react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor)
    );
}

DiscoveryPattern* HybridDisjointDiscovery(PiggybackType hello_piggyback_type, PiggybackType hack_piggyback_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    // TODO: disjoint parameters?

    return newDiscoveryPattern(
        HybridHELLO(hello_piggyback_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor),
        HybridHACK(hack_piggyback_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor)
    );
}

DiscoveryPattern* EchoDiscovery(HackReplyType reply_type, bool piggyback_hello_on_reply, bool react_to_new_neighbor) {
    assert( !piggyback_hello_on_reply || reply_type != UNICAST_HACK_REPLY );

    HelloScheduler* hello_sh = piggyback_hello_on_reply ? HybridHELLO(PIGGYBACK_ON_DISCOVERY_TRAFFIC, react_to_new_neighbor, false, false, false, false, false) : PeriodicHELLO(react_to_new_neighbor, false, false, false, false, false);

    return newDiscoveryPattern(
        hello_sh,
        ReplyHACK(reply_type)
    );
}

HelloScheduler* DP_getHelloScheduler(DiscoveryPattern* dp) {
    assert(dp);
    return dp->hello_sh;
}

HackScheduler* DP_getHackScheduler(DiscoveryPattern* dp) {
    assert(dp);
    return dp->hack_sh;
}
