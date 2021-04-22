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

#ifndef _FORWARDING_STRATEGY_H_
#define _FORWARDING_STRATEGY_H_

#include "../common.h"

typedef struct ForwardingStrategy_ ForwardingStrategy;

void destroyForwardingStrategy(ForwardingStrategy* f_strategy);

bool FS_getNextHop(ForwardingStrategy* f_strategy, RoutingTable* routing_table, SourceTable* source_table, RoutingNeighbors* neighbors, unsigned char* myID, unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, byte** meta_data, unsigned short* meta_data_length, byte* prev_meta_data, unsigned short prev_meta_data_length, bool first, struct timespec* current_time);

///////

ForwardingStrategy* ConventionalRouting();

ForwardingStrategy* SourceRouting();

#endif /* _FORWARDING_STRATEGY_H_ */
