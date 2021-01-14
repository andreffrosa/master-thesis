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

#ifndef _ROUTING_FRAMEWORK_ROUTING_NEIGHBORS_H_
#define _ROUTING_FRAMEWORK_ROUTING_NEIGHBORS_H_

#include "Yggdrasil.h"

typedef struct RoutingNeighbors_ RoutingNeighbors;
typedef struct RoutingNeighborsEntry_ RoutingNeighborsEntry;

RoutingNeighbors* newRoutingNeighbors();

void destroyRoutingNeighbors(RoutingNeighbors* neighbors);

unsigned int RN_getSize(RoutingNeighbors* neighbors);

void RN_addNeighbor(RoutingNeighbors* neighbors, RoutingNeighborsEntry* neigh);

RoutingNeighborsEntry* RN_getNeighbor(RoutingNeighbors* neighbors, unsigned char* neigh_id);

RoutingNeighborsEntry* RN_removeNeighbor(RoutingNeighbors* neighbors, unsigned char* neigh_id);

RoutingNeighborsEntry* RN_nextNeigh(RoutingNeighbors* neighbors, void** iterator);

RoutingNeighborsEntry* newRoutingNeighborsEntry(unsigned char* id, WLANAddr* addr, double rx_cost, double tx_cost, bool is_bi, struct timespec* found_time);

unsigned char* RNE_getID(RoutingNeighborsEntry* neigh);

WLANAddr* RNE_getAddr(RoutingNeighborsEntry* neigh);

double RNE_getRxCost(RoutingNeighborsEntry* neigh);

double RNE_getTxCost(RoutingNeighborsEntry* neigh);

bool RNE_isBi(RoutingNeighborsEntry* neigh);

struct timespec* RNE_getFoundTime(RoutingNeighborsEntry* neigh);

void RNE_setAddr(RoutingNeighborsEntry* neigh, WLANAddr* new_addr);

void RNE_setRxCost(RoutingNeighborsEntry* neigh, double new_rx_cost);

void RNE_setTxCost(RoutingNeighborsEntry* neigh, double new_tx_cost);

void RNE_setBi(RoutingNeighborsEntry* neigh, bool is_bi);

#endif /* _ROUTING_FRAMEWORK_ROUTING_NEIGHBORS_H_ */
