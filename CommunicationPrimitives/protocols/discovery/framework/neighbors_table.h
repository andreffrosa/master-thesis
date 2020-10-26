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

#ifndef _NEIGHBORS_TABLE_H_
#define _NEIGHBORS_TABLE_H_

#include "Yggdrasil.h"

#include "data_structures/hash_table.h"
#include "utility/window.h"

typedef struct _NeighborsTable NeighborsTable;
typedef struct _NeighborEntry NeighborEntry;

typedef enum {
    UNI_NEIGH,
    BI_NEIGH,
    LOST_NEIGH
} DiscoveryNeighborType;

typedef struct _TwoHopNeighbor {
    uuid_t id;
    unsigned short hseq;
    //DiscoveryNeighborType type; // TODO: or bool?
    bool is_symmetric;
    double rx_lq;
    double tx_lq;
    double traffic;
    struct timespec expiration;
} TwoHopNeighbor;

NeighborsTable* newNeighborsTable(unsigned int n_bucket, unsigned int bucket_duration_s);

void destroyNeighborsTable(NeighborsTable* neighbors, void (*lq_destroy)(void*, void*), void (*msg_destroy)(void*, void*), void* lqm, void* d_message);

Window* NT_getOutTraffic(NeighborsTable* neighbors);

Window* NT_getInstability(NeighborsTable* neighbors);

unsigned int NT_getSize(NeighborsTable* neighbors);

void NT_addNeighbor(NeighborsTable* neighbors, NeighborEntry* neigh);

NeighborEntry* NT_getNeighbor(NeighborsTable* neighbors, unsigned char* neigh_id);

NeighborEntry* NT_removeNeighbor(NeighborsTable* neighbors, unsigned char* neigh_id);

NeighborEntry* newNeighborEntry(WLANAddr* mac_addr, unsigned char* id, unsigned short seq, unsigned long hello_period_s, double out_traffic, struct timespec* rx_exp_time, struct timespec* found_time);

void destroyNeighborEntry(NeighborEntry* neigh, void** lq_attributes, void** msg_attributes);

unsigned char* NE_getNeighborID(NeighborEntry* neigh);

WLANAddr* NE_getNeighborMAC(NeighborEntry* neigh);

unsigned short NE_getNeighborSEQ(NeighborEntry* neigh);

void NE_setNeighborSEQ(NeighborEntry* neigh, unsigned short seq);

unsigned short NE_getNeighborHSEQ(NeighborEntry* neigh);

void NE_setNeighborHSEQ(NeighborEntry* neigh, unsigned short hseq);

struct timespec* NE_getNeighborFoundTime(NeighborEntry* neigh);

struct timespec* NE_getNeighborDeletedTime(NeighborEntry* neigh);

bool NE_isDeleted(NeighborEntry* neigh);

void NE_setDeleted(NeighborEntry* neigh, struct timespec* current_time);

unsigned long NE_getNeighborHelloPeriod(NeighborEntry* neigh);

void NE_setNeighborHelloPeriod(NeighborEntry* neigh, unsigned long new_period_s);

unsigned long NE_getNeighborHackPeriod(NeighborEntry* neigh);

void NE_setNeighborHackPeriod(NeighborEntry* neigh, unsigned long new_period_s);

struct timespec* NE_getLastNeighborTimer(NeighborEntry* neigh);

void NE_setLastNeighborTimer(NeighborEntry* neigh, struct timespec* current_time);

struct timespec* NE_getNeighborRxExpTime(NeighborEntry* neigh);

struct timespec* NE_getNeighborTxExpTime(NeighborEntry* neigh);

struct timespec* NE_getNeighborRemovalTime(NeighborEntry* neigh);

DiscoveryNeighborType NE_getNeighborType(NeighborEntry* neigh, struct timespec* current_time);

void NE_setNeighborRxExpTime(NeighborEntry* neigh, struct timespec* rx_exp_time);

void NE_setNeighborTxExpTime(NeighborEntry* neigh, struct timespec* tx_exp_time);

void NE_setNeighborRemovalTime(NeighborEntry* neigh, struct timespec* removal_time);

double NE_getRxLinkQuality(NeighborEntry* neigh);

double NE_getTxLinkQuality(NeighborEntry* neigh);

void NE_setRxLinkQuality(NeighborEntry* neigh, double rx_lq);

void NE_setTxLinkQuality(NeighborEntry* neigh, double tx_lq);

void* NE_setLinkQualityAttributes(NeighborEntry* neigh, void* lq_attributes);

void* NE_getLinkQualityAttributes(NeighborEntry* neigh);

double NE_getOutTraffic(NeighborEntry* neigh);

void NE_setOutTraffic(NeighborEntry* neigh, double traffic);

void* NE_getMessageAttributes(NeighborEntry* neigh);

void* NE_setMessageAttributes(NeighborEntry* neigh, void* msg_attributes);

NeighborEntry* NT_nextNeighbor(NeighborsTable* neighbors, void** iterator);

TwoHopNeighbor* newNeighTwoHopNeighbor(unsigned char* id, unsigned short seq, bool is_symmetric, double rx_lq, double tx_lq, double traffic, struct timespec* expiration);

hash_table* NE_getTwoHopNeighbors(NeighborEntry* neigh);

TwoHopNeighbor* NE_getTwoHopNeighbor(NeighborEntry* neigh, unsigned char* nn_id);

TwoHopNeighbor* NE_removeTwoHopNeighbor(NeighborEntry* neigh, unsigned char* nn_id);

TwoHopNeighbor* NE_addTwoHopNeighbor(NeighborEntry* neigh, TwoHopNeighbor* nn);

char* NT_print(NeighborsTable* neighbors, char** str, struct timespec* current_time, char* window_type, unsigned char* myID, WLANAddr* myMAC, unsigned short my_seq);

#endif /* _NEIGHBORS_TABLE_H_ */
