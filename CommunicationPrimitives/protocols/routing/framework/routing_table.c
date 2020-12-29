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

#include "routing_table.h"

#include "data_structures/hash_table.h"

#include "utility/my_misc.h"
#include "utility/my_string.h"
#include "utility/my_time.h"
#include "utility/my_math.h"

#include <assert.h>

typedef struct _RoutingTable {
    hash_table* ht;
} RoutingTable;

typedef struct _RoutingTableEntry {
    uuid_t destination_id;
    uuid_t next_hop_id;
    WLANAddr next_hop_addr;
    double cost;

    unsigned short seq;

    struct timespec found_time;
    struct timespec last_used_time;

    void* attrs;
    unsigned int attrs_size;
} RoutingTableEntry;

RoutingTable* newRoutingTable() {
    RoutingTable* t = malloc(sizeof(RoutingTable));

    t->ht = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    return t;
}

static void hash_table_delete_custom_fun(hash_table_item* it, void* args) {
    void* attrs = NULL;
    unsigned int attrs_size = 0;
    void (*destroy_attrs)(void*, void*) = args;

    destroyRoutingTableEntry((RoutingTableEntry*)(it->value), &attrs, &attrs_size);

    if( attrs ) {
        if(destroy_attrs)
            destroy_attrs(attrs, NULL);
    }

}

void destroyRoutingTable(RoutingTable* table, void (*destroy_attrs)(void*, void*)) {
    assert(table);

    hash_table_delete_custom(table->ht, &hash_table_delete_custom_fun, destroy_attrs);
    free(table);
}

RoutingTableEntry* RT_addEntry(RoutingTable* table, RoutingTableEntry* entry) {
    assert(table && entry);
    return (RoutingTableEntry*)hash_table_insert(table->ht, entry->destination_id, entry);
}

RoutingTableEntry* RT_findEntry(RoutingTable* table, unsigned char* destination_id) {
    assert(table);

    return (RoutingTableEntry*)hash_table_find_value(table->ht, destination_id);
}

RoutingTableEntry* RT_removeEntry(RoutingTable* table, unsigned char* destination_id) {
    assert(table);

    hash_table_item* it = hash_table_remove(table->ht, destination_id);
    if(it) {
        RoutingTableEntry* entry = (RoutingTableEntry*)it->value;
        free(it);
        return entry;
    } else {
        return NULL;
    }
}

RoutingTableEntry* newRoutingTableEntry(unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, double cost, unsigned short seq, struct timespec* found_time, void* attrs, unsigned int attrs_size) {
    RoutingTableEntry* entry = malloc(sizeof(RoutingTableEntry));

    uuid_copy(entry->destination_id, destination_id);
    uuid_copy(entry->next_hop_id, next_hop_id);
    memcpy(entry->next_hop_addr.data, next_hop_addr->data, WLAN_ADDR_LEN);
    entry->cost = cost;
    entry->seq = seq;

    copy_timespec(&entry->found_time, found_time);
    copy_timespec(&entry->last_used_time, &zero_timespec);

    entry->attrs = attrs;
    entry->attrs_size = attrs_size;

    return entry;
}

void destroyRoutingTableEntry(RoutingTableEntry* entry, void** attrs, unsigned int* attrs_size) {
    if(entry) {
        if(entry->attrs != NULL) {
            *attrs = entry->attrs;
            *attrs_size = entry->attrs_size;
        }
        free(entry);
    }
}

unsigned char* RTE_getDestinationID(RoutingTableEntry* entry) {
    assert(entry);
    return entry->destination_id;
}

unsigned char* RTE_getNextHopID(RoutingTableEntry* entry) {
    assert(entry);
    return entry->next_hop_id;
}

WLANAddr* RTE_getNextHopAddr(RoutingTableEntry* entry) {
    assert(entry);
    return &entry->next_hop_addr;
}

double RTE_getCost(RoutingTableEntry* entry) {
    assert(entry);
    return entry->cost;
}

unsigned short RTE_getSEQ(RoutingTableEntry* entry) {
    assert(entry);
    return entry->seq;
}

struct timespec* RTE_getFoundTime(RoutingTableEntry* entry) {
    assert(entry);
    return &entry->found_time;
}

struct timespec* RTE_getLastUsedTime(RoutingTableEntry* entry) {
    assert(entry);
    return &entry->last_used_time;
}

void* RTE_getAttrs(RoutingTableEntry* entry) {
    assert(entry);
    return entry->attrs;
}

unsigned int RTE_getAttrsSize(RoutingTableEntry* entry) {
    assert(entry);
    return entry->attrs_size;
}

RoutingTableEntry* RT_nextRoute(RoutingTable* table, void** iterator) {
    hash_table_item* item = hash_table_iterator_next(table->ht, iterator);
    if(item) {
        return (RoutingTableEntry*)(item->value);
    } else {
        return NULL;
    }
}



/*
char* RTE_toString(RoutingTableEntry* entry, struct timespec* current_time) {
    unsigned int len = 400; // TODO
    char* str = malloc(len);

    char destination_id_str[UUID_STR_LEN+1];
    destination_id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(entry->destination_id, destination_id_str);

    char next_hop_id_str[UUID_STR_LEN+1];
    next_hop_id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(entry->next_hop_id, next_hop_id_str);

    char next_hop_addr_str[30], next_hop_addr_str2[30];
    wlan2asc(&entry->next_hop_addr, next_hop_addr_str2);
    align_str(next_hop_addr_str, next_hop_addr_str2, 17, "CR");

    assert(entry->cost < 1000);
    unsigned int padding = 3 - (entry->cost == 0 ? 0 : (int)log10(entry->cost));
    char cost_str[20] = {0};
    sprintf(cost_str, "%*s%.2f", padding, "", entry->cost);

    assert(entry->cost < 10000);
    padding = 4 - (entry->other_size == 0 ? 0 : (int)log10(entry->other_size));
    char other_str[20] = {0};
    sprintf(other_str, "%*s%u", padding, "", entry->other_size);

    sprintf(str, "%s   %s   %s   %s     %s\n", destination_id_str, next_hop_id_str, next_hop_addr_str, cost_str, other_str);

    return str;
}
*/

char* RT_toString(RoutingTable* table, char** str, struct timespec* current_time) {
    assert(table && current_time);

    char* header = " # |            DESTINATION ID            |             NEXT HOP ID              |        MAC        |  COST  |  SEQ  |  FOUND  |  USED  |  \n";

    unsigned int line_size = strlen(header) + 1;

    //unsigned int n_routes = table->ht->size;
    unsigned long buffer_size = (table->ht->n_items+1)*line_size;

    char* buffer = malloc(buffer_size);
    char* ptr = buffer;

    // Print Column Headers
    sprintf(ptr, "%s", header);
    ptr += strlen(ptr);

    unsigned int counter = 0;
    void* iterator = NULL;
    RoutingTableEntry* current_route = NULL;
    while( (current_route = RT_nextRoute(table, &iterator)) ) {

        char dest_id_str[UUID_STR_LEN+1];
        uuid_unparse(RTE_getDestinationID(current_route), dest_id_str);
        dest_id_str[UUID_STR_LEN] = '\0';

        char next_hop_id_str[UUID_STR_LEN+1];
        uuid_unparse(RTE_getNextHopID(current_route), next_hop_id_str);
        next_hop_id_str[UUID_STR_LEN] = '\0';

        char next_hop_addr_str[20];
        wlan2asc(RTE_getNextHopAddr(current_route), next_hop_addr_str);
        align_str(next_hop_addr_str, next_hop_addr_str, 17, "CL");

        char seq_str[6];
        sprintf(seq_str, "%hu", RTE_getSEQ(current_route));
        align_str(seq_str, seq_str, 5, "R");

        char cost_str[6];
        sprintf(cost_str, "%0.3f", RTE_getCost(current_route));

        //assert(entry->cost < 1000);
        //unsigned int padding = 3 - (entry->cost == 0 ? 0 : (int)log10(entry->cost));
        //char cost_str[20] = {0};
        //sprintf(cost_str, "%*s%.2f", padding, "", entry->cost);

        struct timespec aux_t = {0};

        char found_time_str[7];
        subtract_timespec(&aux_t, RTE_getFoundTime(current_route), current_time);
        timespec_to_string(&aux_t, found_time_str, 6, 1);
        align_str(found_time_str, found_time_str, 6, "CR");

        char used_time_str[7];
        subtract_timespec(&aux_t, RTE_getLastUsedTime(current_route), current_time);
        timespec_to_string(&aux_t, used_time_str, 6, 1);
        align_str(used_time_str, used_time_str, 6, "CR");


        sprintf(ptr, "%2.d   %s   %s   %s   %s   %s   %s   %s  \n",
        counter+1, dest_id_str, next_hop_id_str, next_hop_addr_str, cost_str, seq_str, found_time_str, used_time_str);
        ptr += strlen(ptr);

        counter++;
    }

    assert(ptr <= buffer+buffer_size);

    *str = buffer;
    return *str;
}
