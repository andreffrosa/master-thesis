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

#include "routing_neighbors.h"

#include "Yggdrasil.h"

#include "data_structures/hash_table.h"

#include "utility/my_misc.h"
#include "utility/my_string.h"
#include "utility/my_time.h"
#include "utility/my_math.h"

#include <assert.h>

typedef struct RoutingNeighbors_ {
    hash_table* ht;
} RoutingNeighbors;

typedef struct RoutingNeighborsEntry_ {
    uuid_t id;
    WLANAddr addr;
    double rx_cost;
    double tx_cost;
    bool is_bi;
    struct timespec found_time;
} RoutingNeighborsEntry;

RoutingNeighbors* newRoutingNeighbors() {
    RoutingNeighbors* rn = malloc(sizeof(RoutingNeighbors));

    rn->ht = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    return rn;
}

static void delete_routing_neighbors_custom(hash_table_item* it, void* args) {
    free(it->value);
}

void destroyRoutingNeighbors(RoutingNeighbors* neighbors) {
    //assert(neighbors);

    if(neighbors) {
        hash_table_delete_custom(neighbors->ht, &delete_routing_neighbors_custom, NULL);
        free(neighbors);
    }
}

unsigned int RN_getSize(RoutingNeighbors* neighbors) {
    assert(neighbors);
    return neighbors->ht->n_items;
}

void RN_addNeighbor(RoutingNeighbors* neighbors, RoutingNeighborsEntry* neigh) {
    assert(neighbors && neigh);

    void* old = hash_table_insert(neighbors->ht, neigh->id, neigh);
    assert(old == NULL);
}

RoutingNeighborsEntry* RN_getNeighbor(RoutingNeighbors* neighbors, unsigned char* neigh_id) {
    assert(neighbors && neigh_id);
    return (RoutingNeighborsEntry*)hash_table_find_value(neighbors->ht, neigh_id);
}

RoutingNeighborsEntry* RN_removeNeighbor(RoutingNeighbors* neighbors, unsigned char* neigh_id) {
    assert(neighbors && neigh_id);

    hash_table_item* it = hash_table_remove_item(neighbors->ht, neigh_id);
    if(it) {
        RoutingNeighborsEntry* entry = (RoutingNeighborsEntry*)it->value;
        free(it);
        return entry;
    } else {
        return NULL;
    }
}

RoutingNeighborsEntry* RN_nextNeigh(RoutingNeighbors* neighbors, void** iterator) {
    hash_table_item* item = hash_table_iterator_next(neighbors->ht, iterator);
    if(item) {
        return (RoutingNeighborsEntry*)(item->value);
    } else {
        return NULL;
    }
}

RoutingNeighborsEntry* newRoutingNeighborsEntry(unsigned char* id, WLANAddr* addr, double rx_cost, double tx_cost, bool is_bi, struct timespec* found_time) {

    RoutingNeighborsEntry* entry = malloc(sizeof(RoutingNeighborsEntry));

    uuid_copy(entry->id, id);
    memcpy(entry->addr.data, addr->data, WLAN_ADDR_LEN);
    entry->rx_cost = rx_cost;
    entry->tx_cost = tx_cost;
    entry->is_bi = is_bi;
    copy_timespec(&entry->found_time, found_time);

    return entry;
}

unsigned char* RNE_getID(RoutingNeighborsEntry* neigh) {
    assert(neigh);
    return neigh->id;
}

WLANAddr* RNE_getAddr(RoutingNeighborsEntry* neigh) {
    assert(neigh);
    return &neigh->addr;
}

double RNE_getRxCost(RoutingNeighborsEntry* neigh) {
    assert(neigh);
    return neigh->rx_cost;
}

double RNE_getTxCost(RoutingNeighborsEntry* neigh) {
    assert(neigh);
    return neigh->tx_cost;
}

bool RNE_isBi(RoutingNeighborsEntry* neigh) {
    assert(neigh);
    return neigh->is_bi;
}

struct timespec* RNE_getFoundTime(RoutingNeighborsEntry* neigh) {
    assert(neigh);
    return &neigh->found_time;
}

void RNE_setAddr(RoutingNeighborsEntry* neigh, WLANAddr* new_addr) {
    assert(neigh && new_addr);
    memcpy(neigh->addr.data, new_addr->data, WLAN_ADDR_LEN);
}

void RNE_setRxCost(RoutingNeighborsEntry* neigh, double new_rx_cost) {
    assert(neigh);
    neigh->rx_cost = new_rx_cost;
}

void RNE_setTxCost(RoutingNeighborsEntry* neigh, double new_tx_cost) {
    assert(neigh);
    neigh->tx_cost = new_tx_cost;
}

void RNE_setBi(RoutingNeighborsEntry* neigh, bool is_bi) {
    assert(neigh);
    neigh->is_bi = is_bi;
}
