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
    //assert( !(hello_sh->type == PIGGYBACK_HELLO) || (hack_sh->type == NO_HACK) );
    assert( !(hack_sh->type == REPLY_HACK) || (hello_sh->type == PERIODIC_HELLO || hello_sh->type == HYBRID_HELLO) );

    DiscoveryPattern* dp = malloc(sizeof(DiscoveryPattern));

    dp->hello_sh = hello_sh;
    dp->hack_sh = hack_sh;

    return dp;
}

DiscoveryPattern* NoDiscovery() {
    return newDiscoveryPattern(NoHELLO(), NoHACK());
}

DiscoveryPattern* PassiveDiscovery(PiggybackFilter* hello_piggyback_filter, PiggybackFilter* hack_piggyback_filter) {
    assert(hello_piggyback_filter && hack_piggyback_filter);

    //HackScheduler* hack_sh = (hack_piggyback_filter == NULL ? NoHACK() : PiggybackHACK(hack_piggyback_filter));

    return newDiscoveryPattern(PiggybackHELLO(hello_piggyback_filter), PiggybackHACK(hack_piggyback_filter));
}

DiscoveryPattern* HybridHelloDiscovery(PiggybackFilter* hello_piggyback_filter, bool react_to_new_neighbor) {
    assert(hello_piggyback_filter);

    return newDiscoveryPattern(
        HybridHELLO(hello_piggyback_filter, react_to_new_neighbor, false, false, false, false, false),
        NoHACK()
    );
}

DiscoveryPattern* PeriodicHelloDiscovery(PeriodicType periodic_type, bool react_to_new_neighbor) {

    return newDiscoveryPattern(
        PeriodicHELLO(periodic_type, react_to_new_neighbor, false, false, false, false, false),
        NoHACK()
    );
}

DiscoveryPattern* PeriodicJointDiscovery(PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    return newDiscoveryPattern(
        PeriodicHELLO(periodic_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor),
        PiggybackHACK(PiggybackOnDiscovery())
    );
}

DiscoveryPattern* PeriodicDisjointDiscovery(PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    // TODO: disjoint parameters?

    return newDiscoveryPattern(
        PeriodicHELLO(periodic_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor),
        PeriodicHACK(periodic_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor)
    );
}

DiscoveryPattern* HybridHelloPeriodicHackDiscovery(PiggybackFilter* hello_piggyback_filter, PeriodicType hack_periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    // TODO: disjoint parameters?

    return newDiscoveryPattern(
        HybridHELLO(hello_piggyback_filter, false /*react_to_new_neighbor*/, false, false, false, false, false), // if hacks react, hellos are piggybacked
        PeriodicHACK(hack_periodic_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor)
    );
}

DiscoveryPattern* PeriodicHelloHybridHackDiscovery(PiggybackFilter* hack_piggyback_filter, PeriodicType hello_periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {
    // TODO: disjoint parameters?

    return newDiscoveryPattern(
        PeriodicHELLO(hello_periodic_type, react_to_new_neighbor, false, false, false, false, false),
        HybridHACK(hack_piggyback_filter, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor)
    );
}

DiscoveryPattern* HybridDisjointDiscovery(PiggybackFilter* hello_piggyback_filter, PiggybackFilter* hack_piggyback_filter, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor) {

    // TODO: disjoint parameters?

    return newDiscoveryPattern(
        HybridHELLO(hello_piggyback_filter, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor),
        HybridHACK(hack_piggyback_filter, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor)
    );
}

DiscoveryPattern* EchoDiscovery(HackReplyType reply_type, bool piggyback_hello_on_reply_if_new_neighbor) {
    assert( !piggyback_hello_on_reply_if_new_neighbor || reply_type != UNICAST_HACK_REPLY );

    HelloScheduler* hello_sh = piggyback_hello_on_reply_if_new_neighbor ? \
    HybridHELLO(PiggybackOnNewNeighbor(), false, false, false, false, false, false) : \
    PeriodicHELLO(STATIC_PERIODIC, false, false, false, false, false, false);

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




DiscoveryInternalEventResult* DP_triggerEvent(DiscoveryPattern* dp, DiscoveryInternalEventType event_type, void* event_args, NeighborsTable* neighbors, YggMessage* msg) {
    assert(dp);

    switch(event_type) {
        case DPE_DOWNSTREAM_MESSAGE: {
            // Piggyback Hello
            PiggybackType hello_piggyback_type = HELLO_evalPiggybackFilter(dp->hello_sh, msg, NULL);

            // Piggyback Hacks
            PiggybackType hack_piggyback_type = HACK_evalPiggybackFilter(dp->hack_sh, msg, NULL);

            NeighborEntry* neigh = NULL;
            WLANAddr* dest_addr = NULL;
            if( hello_piggyback_type == NO_PIGGYBACK && hack_piggyback_type == UNICAST_PIGGYBACK ) {
                // Find Neighbor with the corresponding MAC addr
                neigh = NT_getNeighborByAddr(neighbors, &msg->destAddr);
                if(neigh) {
                    dest_addr = &msg->destAddr;
                }
            }

            DiscoveryInternalEventResult* result = newDiscoveryInternalEventResult(hello_piggyback_type != NO_PIGGYBACK, false, hack_piggyback_type != NO_PIGGYBACK, neigh, dest_addr);

            return result;
        }

        case DPE_HELLO_TIMER: {
            bool request_reply = HACK_replyToHellos(dp->hack_sh) != NO_HACK_REPLY;

            // Piggyback Hacks
            PiggybackType hack_piggyback_type = HACK_evalPiggybackFilter(dp->hack_sh, msg, NULL);

            DiscoveryInternalEventResult* result = newDiscoveryInternalEventResult(true, request_reply, hack_piggyback_type != NO_PIGGYBACK, NULL, NULL);

            return result;
        }

        case DPE_HACK_TIMER: {

            // Piggyback Hello
            PiggybackType hello_piggyback_type = HELLO_evalPiggybackFilter(dp->hello_sh, msg, NULL);

            DiscoveryInternalEventResult* result = newDiscoveryInternalEventResult(hello_piggyback_type != NO_PIGGYBACK, false, true, NULL, NULL);

            return result;
        }

        case DPE_REPLY_TIMER: {
            unsigned char* neigh_id = (unsigned char*)event_args;
            assert(neigh_id);
            NeighborEntry* neigh = NT_getNeighbor(neighbors, neigh_id);
            assert(neigh);

            bool* new_neighbor = (bool*)(event_args + sizeof(uuid_t));

            HackReplyType reply_type = HACK_replyToHellos(dp->hack_sh);
            assert(reply_type != NO_HACK_REPLY);

            // Piggyback Hello
            PiggybackType hello_piggyback_type = HELLO_evalPiggybackFilter(dp->hello_sh, msg, new_neighbor);
            assert(reply_type != UNICAST_HACK_REPLY || hello_piggyback_type == NO_PIGGYBACK );

            WLANAddr* addr = reply_type == UNICAST_HACK_REPLY ? NE_getNeighborMAC(neigh) : NULL;

            DiscoveryInternalEventResult* result = newDiscoveryInternalEventResult(hello_piggyback_type != NO_PIGGYBACK, false, true, neigh, addr);

            return result;
        }

        case DPE_NEIGHBORHOOD_CHANGE_TIMER: {
            NeighborChangeSummary* summary = event_args;

            bool other = summary->other;

            bool new_neighbor = summary->new_neighbor;
            bool lost_neighbor = summary->lost_neighbor;
            bool updated_neighbor = summary->updated_neighbor;

            bool new_2hop_neighbor = summary->added_2hop_neighbor;
            bool lost_2hop_neighbor = summary->lost_2hop_neighbor;
            bool updated_2hop_neighbor = summary->updated_2hop_neighbor;

            bool send_hello = other || \
            (HELLO_newNeighbor(dp->hello_sh) && new_neighbor) || \
            (HELLO_lostNeighbor(dp->hello_sh)  && lost_neighbor) || \
            (HELLO_updateNeighbor(dp->hello_sh) && updated_neighbor) || \
            (HELLO_new2HopNeighbor(dp->hello_sh) && new_2hop_neighbor) || \
            (HELLO_lost2HopNeighbor(dp->hello_sh)  && lost_2hop_neighbor) || \
            (HELLO_update2HopNeighbor(dp->hello_sh) && updated_2hop_neighbor);

            bool send_hack = other || \
            (HACK_newNeighbor(dp->hack_sh) && new_neighbor) || \
            (HACK_lostNeighbor(dp->hack_sh)  && lost_neighbor) || \
            (HACK_updateNeighbor(dp->hack_sh) && updated_neighbor) || \
            (HACK_new2HopNeighbor(dp->hack_sh) && new_2hop_neighbor) || \
            (HACK_lost2HopNeighbor(dp->hack_sh)  && lost_2hop_neighbor) || \
            (HACK_update2HopNeighbor(dp->hack_sh) && updated_2hop_neighbor);

            if( send_hello || send_hack ) {
                if(!send_hello) {
                    // Piggyback Hello
                    PiggybackType hello_piggyback_type = HELLO_evalPiggybackFilter(dp->hello_sh, msg, NULL);
                    send_hello = hello_piggyback_type != NO_PIGGYBACK;
                }

                if(!send_hack) {
                    // Piggyback Hacks
                    PiggybackType hack_piggyback_type = HACK_evalPiggybackFilter(dp->hack_sh, msg, NULL);
                    send_hack = hack_piggyback_type != NO_PIGGYBACK;
                }
            }

            DiscoveryInternalEventResult* result = newDiscoveryInternalEventResult(send_hello, false, send_hack, NULL, NULL);

            return result;
        }

        default: return NULL;
    }

}
