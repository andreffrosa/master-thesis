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

typedef struct RoutingTable_ {
    hash_table* ht;
} RoutingTable;

typedef struct RoutingTableEntry_ {
    uuid_t destination_id;
    uuid_t next_hop_id;
    WLANAddr next_hop_addr;

    double cost;
    unsigned int hops;

    //unsigned short seq;

    unsigned long messages_forwarded;

    struct timespec found_time;
    struct timespec last_used_time;

    //void* attrs;
    //unsigned int attrs_size;
} RoutingTableEntry;

RoutingTable* newRoutingTable() {
    RoutingTable* t = malloc(sizeof(RoutingTable));

    t->ht = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    return t;
}

static void hash_table_delete_custom_fun(hash_table_item* it, void* args) {
    //void* attrs = NULL;
    //unsigned int attrs_size = 0;
    //void (*destroy_attrs)(void*, void*) = args;

    destroyRoutingTableEntry((RoutingTableEntry*)(it->value)/*, &attrs, &attrs_size*/);

    //if( attrs ) {
    //    if(destroy_attrs)
    //        destroy_attrs(attrs, NULL);
    //}
}

void destroyRoutingTable(RoutingTable* rt/*, void (*destroy_attrs)(void*, void*)*/) {
    assert(rt);

    hash_table_delete_custom(rt->ht, &hash_table_delete_custom_fun, NULL /*destroy_attrs*/);
    free(rt);
}

RoutingTableEntry* RT_addEntry(RoutingTable* rt, RoutingTableEntry* entry) {
    assert(rt && entry);
    return (RoutingTableEntry*)hash_table_insert(rt->ht, entry->destination_id, entry);
}

RoutingTableEntry* RT_findEntry(RoutingTable* rt, unsigned char* destination_id) {
    assert(rt);

    return (RoutingTableEntry*)hash_table_find_value(rt->ht, destination_id);
}

RoutingTableEntry* RT_removeEntry(RoutingTable* rt, unsigned char* destination_id) {
    assert(rt);

    hash_table_item* it = hash_table_remove_item(rt->ht, destination_id);
    if(it) {
        RoutingTableEntry* entry = (RoutingTableEntry*)it->value;
        free(it);
        return entry;
    } else {
        return NULL;
    }
}

bool RT_update(RoutingTable* rt, list* to_update, list* to_remove) {
    assert(rt);

    bool updated = false;

    if(to_update) {
        RoutingTableEntry* new_entry = NULL;
        while( (new_entry = list_remove_head(to_update)) ) {

            RoutingTableEntry* old_entry = RT_removeEntry(rt, new_entry->destination_id);
            if(old_entry) {

                if(uuid_compare(new_entry->next_hop_id, old_entry->next_hop_id) == 0) {
                    copy_timespec(&new_entry->found_time, &old_entry->found_time);
                    copy_timespec(&new_entry->last_used_time, &old_entry->last_used_time);
                    new_entry->messages_forwarded = old_entry->messages_forwarded;
                } else {
                    RTE_resetMessagesForwarded(new_entry);
                    //RTE_setFoundTime(current_entry, current_time);
                    RTE_setLastUsedTime(new_entry, (struct timespec*)&zero_timespec);
                }

                RT_addEntry(rt, new_entry);

                free(old_entry);

                updated = true;
            } else {
                RT_addEntry(rt, new_entry);

                updated = true;
            }
        }

        list_delete(to_update);
    }

    if(to_remove) {
        unsigned char* id = NULL;
        while( (id = list_remove_head(to_remove)) ) {
            RoutingTableEntry* old_entry = RT_removeEntry(rt, id);
            free(old_entry);

            free(id);

            updated = true;
        }

        free(to_remove);
    }

    return updated;
}

RoutingTableEntry* newRoutingTableEntry(unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, double cost, unsigned int hops, struct timespec* found_time) {
    RoutingTableEntry* entry = malloc(sizeof(RoutingTableEntry));

    uuid_copy(entry->destination_id, destination_id);
    uuid_copy(entry->next_hop_id, next_hop_id);
    memcpy(entry->next_hop_addr.data, next_hop_addr->data, WLAN_ADDR_LEN);
    entry->cost = cost;
    entry->hops = hops;
    //entry->seq = seq;

    copy_timespec(&entry->found_time, found_time);
    copy_timespec(&entry->last_used_time, &zero_timespec);

    entry->messages_forwarded = 0;

    //entry->attrs = attrs;
    //entry->attrs_size = attrs_size;

    return entry;
}

void destroyRoutingTableEntry(RoutingTableEntry* entry /*, void** attrs, unsigned int* attrs_size*/) {
    if(entry) {
        /*if(entry->attrs != NULL) {
            *attrs = entry->attrs;
            *attrs_size = entry->attrs_size;
        }*/
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

void RTE_setNexHop(RoutingTableEntry* entry, unsigned char* id, WLANAddr* addr) {
    assert(entry);
    uuid_copy(entry->next_hop_id, id);
    memcpy(entry->next_hop_addr.data, addr->data, WLAN_ADDR_LEN);
}

double RTE_getCost(RoutingTableEntry* entry) {
    assert(entry);
    return entry->cost;
}

void RTE_setCost(RoutingTableEntry* entry, double new_cost) {
    assert(entry);
    entry->cost = new_cost;
}

unsigned int RTE_getHops(RoutingTableEntry* entry) {
    assert(entry);
    return entry->hops;
}

void RTE_setHops(RoutingTableEntry* entry, unsigned int new_hops) {
    assert(entry);
    entry->hops = new_hops;
}

/*unsigned short RTE_getSEQ(RoutingTableEntry* entry) {
    assert(entry);
    return entry->seq;
}*/

unsigned long RTE_getMessagesForwarded(RoutingTableEntry* entry) {
    assert(entry);
    return entry->messages_forwarded;
}

void RTE_incMessagesForwarded(RoutingTableEntry* entry) {
    assert(entry);
    entry->messages_forwarded++;
}

void RTE_resetMessagesForwarded(RoutingTableEntry* entry) {
    assert(entry);
    entry->messages_forwarded = 0;
}

struct timespec* RTE_getFoundTime(RoutingTableEntry* entry) {
    assert(entry);
    return &entry->found_time;
}

void RTE_setFoundTime(RoutingTableEntry* entry, struct timespec* t) {
    assert(entry);
    copy_timespec(&entry->found_time, t);
}

struct timespec* RTE_getLastUsedTime(RoutingTableEntry* entry) {
    assert(entry);
    return &entry->last_used_time;
}

void RTE_setLastUsedTime(RoutingTableEntry* entry, struct timespec* t) {
    assert(entry);
    copy_timespec(&entry->last_used_time, t);
}

/*
void* RTE_getAttrs(RoutingTableEntry* entry) {
    assert(entry);
    return entry->attrs;
}

unsigned int RTE_getAttrsSize(RoutingTableEntry* entry) {
    assert(entry);
    return entry->attrs_size;
}
*/

RoutingTableEntry* RT_nextRoute(RoutingTable* rt, void** iterator) {
    hash_table_item* item = hash_table_iterator_next(rt->ht, iterator);
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

char* RT_toString(RoutingTable* rt, char** str, struct timespec* current_time) {
    assert(rt && current_time);

    char* header = " # |            DESTINATION ID            |             NEXT HOP ID              |    NEXT HOP MAC   |  COST  |  HOPS  |  FOUND  |  USED  |  FORWARDED  |  \n";

    unsigned int line_size = strlen(header) + 1;

    //unsigned int n_routes = rt->ht->size;
    unsigned long buffer_size = (rt->ht->n_items+1)*line_size;

    char* buffer = malloc(buffer_size);
    char* ptr = buffer;

    // Print Column Headers
    sprintf(ptr, "%s", header);
    ptr += strlen(ptr);

    unsigned int counter = 0;
    void* iterator = NULL;
    RoutingTableEntry* current_route = NULL;
    while( (current_route = RT_nextRoute(rt, &iterator)) ) {

        char dest_id_str[UUID_STR_LEN+1];
        uuid_unparse(RTE_getDestinationID(current_route), dest_id_str);
        dest_id_str[UUID_STR_LEN] = '\0';

        char next_hop_id_str[UUID_STR_LEN+1];
        uuid_unparse(RTE_getNextHopID(current_route), next_hop_id_str);
        next_hop_id_str[UUID_STR_LEN] = '\0';

        char next_hop_addr_str[20];
        wlan2asc(RTE_getNextHopAddr(current_route), next_hop_addr_str);
        align_str(next_hop_addr_str, next_hop_addr_str, 17, "CL");

        /*
        char seq_str[6];
        sprintf(seq_str, "%hu", RTE_getSEQ(current_route));
        align_str(seq_str, seq_str, 5, "R");
        */

        char cost_str[6];
        sprintf(cost_str, "%0.3f", RTE_getCost(current_route));

        char hops_str[6];
        sprintf(hops_str, "%u", RTE_getHops(current_route));
        align_str(hops_str, hops_str, 5, "R");

        //assert(entry->cost < 1000);
        //unsigned int padding = 3 - (entry->cost == 0 ? 0 : (int)log10(entry->cost));
        //char cost_str[20] = {0};
        //sprintf(cost_str, "%*s%.2f", padding, "", entry->cost);

        struct timespec aux_t = {0};

        char found_time_str[7];
        subtract_timespec(&aux_t, current_time, RTE_getFoundTime(current_route));
        timespec_to_string(&aux_t, found_time_str, 6, 1);
        align_str(found_time_str, found_time_str, 6, "CR");

        char used_time_str[7];
        if( compare_timespec(RTE_getLastUsedTime(current_route), (struct timespec*)&zero_timespec) != 0) {
            subtract_timespec(&aux_t, current_time, RTE_getLastUsedTime(current_route));
            timespec_to_string(&aux_t, used_time_str, 6, 1);
            align_str(used_time_str, used_time_str, 6, "CR");
        } else {
            sprintf(used_time_str, "   -  ");
        }

        char forwarded_str[12];
        sprintf(forwarded_str, "%lu", RTE_getMessagesForwarded(current_route));
        align_str(forwarded_str, forwarded_str, 11, "R");

        sprintf(ptr, "%2.d   %s   %s   %s   %s     %s    %s   %s   %s  \n",
        counter+1, dest_id_str, next_hop_id_str, next_hop_addr_str, cost_str, hops_str, found_time_str, used_time_str, forwarded_str);
        ptr += strlen(ptr);

        counter++;
    }

    assert(ptr <= buffer+buffer_size);

    *str = buffer;
    return *str;
}
