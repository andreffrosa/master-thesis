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

#include "routing_context_private.h"

#include <assert.h>

#define ID_TEMPLATE "66600666-1001-1001-1001-000000000001"

const char* mac_addrs[] = {
    "b8:27:eb:3a:96:e",
    "b8:27:eb:9a:68:47",
    "b8:27:eb:38:94:8d",
    "b8:27:eb:7e:f4:68",
    "b8:27:eb:90:75:4f",
    "b8:27:eb:f:d:28",
    "b8:27:eb:cd:8a:c1",
    "b8:27:eb:85:b1:84",
    "b8:27:eb:6f:1e:4",
    "b8:27:eb:6:f6:c3",
    "b8:27:eb:63:63:c7",
    "b8:27:eb:c5:fb:2e",
    "b8:27:eb:78:77:7",
    "b8:27:eb:1a:eb:77",
    "b8:27:eb:97:3b:51",
    "b8:27:eb:c8:3e:ad",
    "b8:27:eb:b5:2e:4d",
    "b8:27:eb:76:9a:8",
    "b8:27:eb:55:a5:1d",
    "b8:27:eb:6a:1e:c4",
    "b8:27:eb:31:45:24",
    "b8:27:eb:de:1b:a4",
    "b8:27:eb:f5:55:35",
    "b8:27:eb:39:79:d3"
};

static void parseNode(unsigned char* id, WLANAddr* addr, unsigned int node_id) {
    char id_str[UUID_STR_LEN+1];
    strcpy(id_str, ID_TEMPLATE);

    char aux[10];
    sprintf(aux, "%u", node_id);
    int len = strlen(aux);

    char* ptr = id_str + strlen(id_str) - len;
    memcpy(ptr, aux, len);

    //printf("\t\t\nID: %s MAC: %s\n\n", id_str, mac_addrs[node_id-1]);
    //fflush(stdout);

    int a = uuid_parse(id_str, id);
    assert(a >= 0);

    str2wlan((char*)addr->data, (char*)mac_addrs[node_id-1]);
}

static void addEntry(unsigned int destination, unsigned int next_hop, double cost, unsigned int hops, struct timespec* found_time, list* entries, uuid_t node_ids[], WLANAddr node_addrs[], const char* proto) {

    unsigned char* destination_id = node_ids[destination-1];
    unsigned char* next_hop_id = node_ids[next_hop-1];
    WLANAddr* next_hop_addr = &node_addrs[next_hop-1];

    RoutingTableEntry* new_entry = newRoutingTableEntry(destination_id, next_hop_id, next_hop_addr, cost, hops, found_time, proto);

    //RoutingTableEntry* old_entry = RT_addEntry(routing_table, new_entry);
    //assert(old_entry == NULL);
    list_add_item_to_tail(entries, new_entry);
}

static void StaticRoutingContextInit(ModuleState* context_state, const char* proto, proto_def* protocol_definition, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time) {

    unsigned int amount = 9;
    uuid_t node_ids[amount];
    WLANAddr node_addrs[amount];

    for(int i = 0; i < amount; i++) {
        parseNode(node_ids[i], &node_addrs[i], i+1);
    }

    list* to_update = list_init();

    int myId = myID[15];
    switch(myId) {
        case 1: {
            addEntry(2, 2, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(3, 3, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(4, 3, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(5, 3, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(6, 3, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(7, 3, 5, 5, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(8, 3, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(9, 3, 5, 5, current_time, to_update, node_ids, node_addrs, proto);
        }
        break;
        case 2: {
            addEntry(1, 1, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(3, 3, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(4, 3, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(5, 3, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(6, 3, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(7, 3, 5, 5, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(8, 3, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(9, 3, 5, 5, current_time, to_update, node_ids, node_addrs, proto);
        }
        break;
        case 3: {
            addEntry(1, 1, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(2, 2, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(4, 4, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(5, 4, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(6, 4, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(7, 4, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(8, 4, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(9, 4, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
        }
        break;
        case 4: {
            addEntry(1, 3, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(2, 3, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(3, 3, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(5, 5, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(6, 5, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(7, 5, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(8, 5, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(9, 5, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
        }
        break;
        case 5: {
            addEntry(1, 4, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(2, 4, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(3, 4, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(4, 4, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(6, 6, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(7, 6, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(8, 8, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(9, 8, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
        }
        break;
        case 6: {
            addEntry(1, 5, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(2, 5, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(3, 5, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(4, 5, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(5, 5, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(7, 7, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(8, 7, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(9, 7, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
        }
        break;
        case 7: {
            addEntry(1, 6, 5, 5, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(2, 6, 5, 5, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(3, 6, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(4, 8, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(5, 8, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(6, 6, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(8, 8, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(9, 8, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
        }
        break;
        case 8: {
            addEntry(1, 5, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(2, 5, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(3, 5, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(4, 5, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(5, 5, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(6, 7, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(7, 7, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(9, 9, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
        }
        break;
        case 9: {
            addEntry(1, 8, 5, 5, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(2, 8, 5, 5, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(3, 8, 4, 4, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(4, 8, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(5, 8, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(6, 8, 3, 3, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(7, 8, 2, 2, current_time, to_update, node_ids, node_addrs, proto);
            addEntry(8, 8, 1, 1, current_time, to_update, node_ids, node_addrs, proto);
        }
        break;
        default: assert(false);
    }

    RF_updateRoutingTable(routing_table, to_update, NULL, current_time);
}

RoutingContext* StaticRoutingContext() {

    return newRoutingContext(
        "STATIC",
        NULL,
        NULL,
        &StaticRoutingContextInit,
        NULL,
        NULL,
        NULL,
        NULL
    );
}
