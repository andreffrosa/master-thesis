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

#ifndef _DISCOVERY_MESSAGE_H_
#define _DISCOVERY_MESSAGE_H_

#include "../common.h"

typedef struct _DiscoveryMessage DiscoveryMessage;

void destroyDiscoveryMessage(DiscoveryMessage* dm);

DiscoveryMessage* SimpleDiscoveryMessage();

DiscoveryMessage* OLSRDiscoveryMessage();

// DiscoveryMessage* TopologyDiscoveryMessage();

void DM_create(DiscoveryMessage* dm, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size);

bool DM_process(DiscoveryMessage* dm, void* f_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size);

void* DM_createAttrs(DiscoveryMessage* dm);

void DM_destroyAttrs(DiscoveryMessage* dm, void* msg_attributes);

#endif /* _DISCOVERY_MESSAGE_H_ */
