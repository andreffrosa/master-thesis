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

#include "forwarding_strategy_private.h"

#include <assert.h>

static bool SourceRouting_getNextHop(ModuleState* m_state, RoutingTable* routing_table, SourceTable* source_table, RoutingNeighbors* neighbors, unsigned char* myID, unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, byte** meta_data, unsigned short* meta_data_length, byte* prev_meta_data, unsigned short prev_meta_data_length, bool first, struct timespec* current_time) {

    if(first) {
        SourceEntry* source_entry = ST_getEntry(source_table, destination_id);

        if(source_entry == NULL) {
            printf("[getNextHop] No source entry\n");
            return false;
        }

        list* route = (list*)SE_getAttr(source_entry, "route");
        if(route) {

            char aux_str[UUID_STR_LEN];
            uuid_unparse(SE_getID(source_entry), aux_str);
            printf("[getNextHop] Found route to %s (%d):\n", aux_str, route->size);

            *meta_data_length = route->size*sizeof(uuid_t);
            *meta_data = malloc(*meta_data_length);
            byte* ptr = *meta_data;

            for(list_item* it = route->head; it; it = it->next) {
                memcpy(ptr, it->data, sizeof(uuid_t));
                ptr += sizeof(uuid_t);

                char id_str[UUID_STR_LEN];
                uuid_unparse(ptr, id_str);
                printf("=> %s\n", id_str);
            }

            RoutingTableEntry* entry = RT_findEntry(routing_table, destination_id);
            assert(entry);
            //if(entry) {
                uuid_copy(next_hop_id, RTE_getNextHopID(entry));
                memcpy(next_hop_addr->data, RTE_getNextHopAddr(entry)->data, WLAN_ADDR_LEN);

                RTE_setLastUsedTime(entry, current_time);
                RTE_incMessagesForwarded(entry);

                printf("[getNextHop] Found route in routing table!\n");

                return true;
            /*}
            else {
                return false;
            }*/
        } else {
            printf("[getNextHop] No route in source_entry\n");

            // send RREQ
            return false;
        }
    } else {
        if(prev_meta_data) {
            *meta_data_length = prev_meta_data_length;
            *meta_data = malloc(*meta_data_length);
            memcpy(*meta_data, prev_meta_data, *meta_data_length);

            bool found = false;

            int n = prev_meta_data_length / sizeof(uuid_t);

            printf("[SOURCE ROUTING] route_length=%d\n", n);

            byte* ptr = prev_meta_data;
            for(int i = 0; i < n - 1; i++) {
                unsigned char* id = ptr + i*sizeof(uuid_t);
                if( uuid_compare(myID, id) == 0 ) {
                    //assert( i + sizeof(uuid_t) < prev_meta_data_length );
                    //assert(i < n - 1);
                    uuid_copy(next_hop_id, ptr + (i+1)*sizeof(uuid_t));
                    found = true;
                    break;
                }
            }

            if(found) {
                RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, next_hop_id);
                if(neigh) {
                    memcpy(next_hop_addr->data, RNE_getAddr(neigh)->data, WLAN_ADDR_LEN);

                    printf("[getNextHop] Found!\n");

                    return true;
                }
            }

            printf("[getNextHop] Cannot find myself in route!\n");

            return false;
        } else {
            assert(false);
        }
    }

}

ForwardingStrategy* SourceRouting() {
    return newForwardingStrategy(
        NULL,
        NULL,
        &SourceRouting_getNextHop,
        NULL
    );
}
