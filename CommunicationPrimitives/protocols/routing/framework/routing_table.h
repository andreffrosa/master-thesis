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

#ifndef _ROUTING_FRAMEWORK_ROUTING_TABLE_H_
#define _ROUTING_FRAMEWORK_ROUTING_TABLE_H_

#include "Yggdrasil.h"

#include "data_structures/list.h"

typedef struct RoutingTable_ RoutingTable;
typedef struct RoutingTableEntry_ RoutingTableEntry;

RoutingTable* newRoutingTable();

void destroyRoutingTable(RoutingTable* rt);

RoutingTableEntry* RT_addEntry(RoutingTable* rt, RoutingTableEntry* entry);

RoutingTableEntry* RT_findEntry(RoutingTable* rt, unsigned char* destination_id);

RoutingTableEntry* RT_removeEntry(RoutingTable* rt, unsigned char* destination_id);

bool RT_update(RoutingTable* rt, list* to_update, list* to_remove);

RoutingTableEntry* newRoutingTableEntry(unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, double cost, unsigned int hops, struct timespec* found_time);

RoutingTableEntry* RT_nextRoute(RoutingTable* rt, void** iterator);

void destroyRoutingTableEntry(RoutingTableEntry* entry);

unsigned char* RTE_getDestinationID(RoutingTableEntry* entry);

unsigned char* RTE_getNextHopID(RoutingTableEntry* entry);

WLANAddr* RTE_getNextHopAddr(RoutingTableEntry* entry);

void RTE_setNexHop(RoutingTableEntry* entry, unsigned char* id, WLANAddr* addr);

double RTE_getCost(RoutingTableEntry* entry);

void RTE_setCost(RoutingTableEntry* entry, double new_cost);

unsigned int RTE_getHops(RoutingTableEntry* entry);

void RTE_setHops(RoutingTableEntry* entry, unsigned int new_hops);

//unsigned short RTE_getSEQ(RoutingTableEntry* entry);

unsigned long RTE_getMessagesForwarded(RoutingTableEntry* entry);

void RTE_incMessagesForwarded(RoutingTableEntry* entry);

void RTE_resetMessagesForwarded(RoutingTableEntry* entry);

struct timespec* RTE_getFoundTime(RoutingTableEntry* entry);

void RTE_setFoundTime(RoutingTableEntry* entry, struct timespec* t);

struct timespec* RTE_getLastUsedTime(RoutingTableEntry* entry);

void RTE_setLastUsedTime(RoutingTableEntry* entry, struct timespec* t);

/*
void* RTE_getAttrs(RoutingTableEntry* entry);

unsigned int RTE_getAttrsSize(RoutingTableEntry* entry);

char* RTE_toString(RoutingTableEntry* entry, struct timespec* current_time);
*/

char* RT_toString(RoutingTable* rt, char** str, struct timespec* current_time);

#endif /* _ROUTING_FRAMEWORK_ROUTING_TABLE_H_ */
