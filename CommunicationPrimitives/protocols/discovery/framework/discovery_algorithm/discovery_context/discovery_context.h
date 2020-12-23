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

#ifndef _DISCOVERY_CONTEXT_H_
#define _DISCOVERY_CONTEXT_H_

#include "../common.h"

typedef struct DiscoveryContext_ DiscoveryContext;

void destroyDiscoveryContext(DiscoveryContext* d_ctx);

void DC_create(DiscoveryContext* d_ctx, unsigned char* myID, NeighborsTable* neighbors, DiscoveryInternalEventType event_type, void* event_args, struct timespec* current_time, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size);

bool DC_process(DiscoveryContext* d_ctx, void* f_state, unsigned char* myID, NeighborsTable* neighbors, struct timespec* current_time, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size, MessageSummary* msg_summary);

bool DC_update(DiscoveryContext* d_ctx, unsigned char* myID, NeighborEntry* neighbor, NeighborsTable* neighbors, struct timespec* current_time, NeighborTimerSummary* summary);

void* DC_createAttrs(DiscoveryContext* d_ctx);

void DC_destroyAttrs(DiscoveryContext* d_ctx, void* msg_attributes);

//////////////////////////////////////////////////////////

DiscoveryContext* EmptyDiscoveryContext();

DiscoveryContext* OLSRDiscoveryContext();

DiscoveryContext* LENWBDiscoveryContext();


#endif /*_DISCOVERY_CONTEXT_H_*/
