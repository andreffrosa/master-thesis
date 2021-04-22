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

static bool ConventionalRouting_getNextHop(ModuleState* m_state, RoutingTable* routing_table, SourceTable* source_table, RoutingNeighbors* neighbors, unsigned char* myID, unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, byte** meta_data, unsigned short* meta_data_length, byte* prev_meta_data, unsigned short prev_meta_data_length, bool first, struct timespec* current_time) {

    RoutingTableEntry* entry = RT_findEntry(routing_table, destination_id);
    if(entry) {
        uuid_copy(next_hop_id, RTE_getNextHopID(entry));
        memcpy(next_hop_addr->data, RTE_getNextHopAddr(entry)->data, WLAN_ADDR_LEN);

        RTE_setLastUsedTime(entry, current_time);
        RTE_incMessagesForwarded(entry);

        return true;
    } else {
        return false;
    }

}

ForwardingStrategy* ConventionalRouting() {
    return newForwardingStrategy(
        NULL,
        NULL,
        &ConventionalRouting_getNextHop,
        NULL
    );
}
