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

typedef struct _RoutingTable RoutingTable;
typedef struct _RoutingTableEntry RoutingTableEntry;

RoutingTable* newRoutingTable();

void destroyRoutingTable(RoutingTable* table, void (*destroy_attrs)(void*, void*));

RoutingTableEntry* RT_addEntry(RoutingTable* table, RoutingTableEntry* entry);

RoutingTableEntry* RT_findEntry(RoutingTable* table, unsigned char* destination_id);

RoutingTableEntry* RT_removeEntry(RoutingTable* table, unsigned char* destination_id);

RoutingTableEntry* newRoutingTableEntry(unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, double cost, unsigned short seq, struct timespec* found_time, void* attrs, unsigned int attrs_size);

RoutingTableEntry* RT_nextRoute(RoutingTable* table, void** iterator);

void destroyRoutingTableEntry(RoutingTableEntry* entry, void** attrs, unsigned int* attrs_size);

unsigned char* RTE_getDestinationID(RoutingTableEntry* entry);

unsigned char* RTE_getNextHopID(RoutingTableEntry* entry);

WLANAddr* RTE_getNextHopAddr(RoutingTableEntry* entry);

double RTE_getCost(RoutingTableEntry* entry);

unsigned short RTE_getSEQ(RoutingTableEntry* entry);

struct timespec* RTE_getFoundTime(RoutingTableEntry* entry);

struct timespec* RTE_getLastUsedTime(RoutingTableEntry* entry);

void* RTE_getAttrs(RoutingTableEntry* entry);

unsigned int RTE_getAttrsSize(RoutingTableEntry* entry);
/*
char* RTE_toString(RoutingTableEntry* entry, struct timespec* current_time);
*/

char* RT_toString(RoutingTable* table, char** str, struct timespec* current_time);

#endif /* _ROUTING_FRAMEWORK_ROUTING_TABLE_H_ */
