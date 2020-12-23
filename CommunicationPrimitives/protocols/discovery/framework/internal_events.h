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

#ifndef _DISCOVERY_INTERNAL_EVENTS_H_
#define _DISCOVERY_INTERNAL_EVENTS_H_

#include "neighbors_table.h"
#include "messages.h"

typedef enum {
    DPE_DOWNSTREAM_MESSAGE,
    //DPE_UPSTREAM_MESSAGE,
    DPE_HELLO_TIMER,
    DPE_HACK_TIMER,
    DPE_NEIGHBORHOOD_CHANGE_TIMER,
    DPE_REPLY_TIMER
} DiscoveryInternalEventType;

typedef struct DiscoveryEventResult_ {
    bool create_hello;
    bool request_replies;
    bool create_hack;
    NeighborEntry* hack_dest;
    //bool is_unicast;
    WLANAddr dest_addr;
} DiscoveryInternalEventResult;

typedef struct DiscoverySendPack_ {
    HelloMessage* hello;
    unsigned int n_hacks;
    HackMessage* hacks;
} DiscoverySendPack;


DiscoveryInternalEventResult* newDiscoveryInternalEventResult(bool create_hello, bool request_replies, bool create_hack, NeighborEntry* hack_dest, WLANAddr* dest_addr);

DiscoverySendPack* newDiscoverySendPack(HelloMessage* hello, unsigned int n_hacks, HackMessage* hacks);


#endif /* _DISCOVERY_INTERNAL_EVENTS_H_ */
