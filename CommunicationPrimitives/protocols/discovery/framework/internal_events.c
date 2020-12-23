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

#include "internal_events.h"

// #include "Yggdrasil.h"

#include <assert.h>

DiscoveryInternalEventResult* newDiscoveryInternalEventResult(bool create_hello, bool request_replies, bool create_hack, NeighborEntry* hack_dest, WLANAddr* dest_addr) {
    assert(create_hack || hack_dest == NULL);
    assert(dest_addr == NULL || !create_hello);

    DiscoveryInternalEventResult* result = malloc(sizeof(DiscoveryInternalEventResult));

    result->create_hello = create_hello;
    result->request_replies = request_replies;
    result->create_hack = create_hack;
    result->hack_dest = hack_dest;
    //result->dest_addr = dest_addr;
    //result->is_unicast = is_unicast;

    if(dest_addr) {
        memcpy(result->dest_addr.data, dest_addr->data, WLAN_ADDR_LEN);
    } else {
        memset(result->dest_addr.data, 0, WLAN_ADDR_LEN);
    }

    return result;
}

DiscoverySendPack* newDiscoverySendPack(HelloMessage* hello, unsigned int n_hacks, HackMessage* hacks) {
    assert((n_hacks > 0 || hacks == NULL) && (hacks != NULL || n_hacks == 0));

    DiscoverySendPack* dsp = malloc(sizeof(DiscoverySendPack));

    dsp->hello = hello;
    dsp->n_hacks = n_hacks;
    dsp->hacks = hacks;

    return dsp;
}
