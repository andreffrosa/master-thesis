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

static bool getBestBiParent(RoutingNeighbors* neighbors, byte* meta_data, unsigned int meta_length, unsigned char* found_parent, double* found_route_cost, unsigned int* found_route_hops);

static void RecomputeRoutingTable(unsigned char* source_id, unsigned char* parent_id, double cost, unsigned int hops, RoutingNeighbors* neighbors, RoutingTable* routing_table, struct timespec* current_time);

static void AODVRoutingContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time) {

}

static RoutingContextSendType AODVRoutingContextTriggerEvent(ModuleState* m_state, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {

    if(event_type == RTE_ROUTE_NOT_FOUND) {
        printf("SENDING RREQ\n");
        return SEND_INC;
    }

    return NO_SEND;
}

static void AODVRoutingContextCreateMsg(ModuleState* m_state, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, void* info) {

    byte type = 1; // RREQ TODO colocar em enum
    YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

    assert(info);
    unsigned char* destination_id = info;
    YggMessage_addPayload(msg, (char*)destination_id, sizeof(uuid_t));

}

static void AODVRoutingContextProcessMsg(ModuleState* m_state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, byte* meta_data, unsigned int meta_length) {

    void* ptr = payload;

    byte type = 0;
    memcpy(&type, ptr, sizeof(byte));
    ptr += sizeof(byte);

    uuid_t destination_id;
    memcpy(destination_id, ptr, sizeof(uuid_t));
    ptr += sizeof(uuid_t);

    // TODO: como ir buscar o custo do RREQ no cabeçalho de bcast e as cópias?
    // TODO: adicionar/atualizar rota para a source do RREQ
    uuid_t found_parent = {0};
    double found_route_cost = 0.0;
    unsigned int found_route_hops = 0;
    bool found = getBestBiParent(neighbors, meta_data, meta_length, found_parent, &found_route_cost, &found_route_hops);
    if( found ) {
        // Update Routing Table
        RecomputeRoutingTable(SE_getID(source_entry), found_parent, found_route_cost, found_route_hops, neighbors, routing_table, current_time);

        if( uuid_compare(destination_id, myID) == 0 ) {
            // TODO: send RREP
            // TODO: como fazer trigger do envio de uma RREP? --> a recepção passa a ser um evento que dá trigger também para além do timer, neigh change, ...

            printf("I'm the wanted destination!\n");
        }
    }
}

//typedef void (*rc_destroy)(ModuleState* m_state);

RoutingContext* AODVRoutingContext() {

    return newRoutingContext(
        NULL,
        NULL,
        &AODVRoutingContextInit,
        &AODVRoutingContextTriggerEvent,
        &AODVRoutingContextCreateMsg,
        &AODVRoutingContextProcessMsg,
        NULL
    );
}

static bool getBestBiParent(RoutingNeighbors* neighbors, byte* meta_data, unsigned int meta_length, unsigned char* found_parent, double* found_route_cost, unsigned int* found_route_hops) {

    uuid_t min_parent_id = {0};
    double min_route_cost = 0.0;
    unsigned int min_route_hops = 0;
    bool first = true;


    byte* ptr2 = meta_data;

    uuid_t source_id;
    memcpy(source_id, ptr2, sizeof(uuid_t));
    ptr2 += sizeof(uuid_t);

    byte n_copies = 0;
    memcpy(&n_copies, ptr2, sizeof(byte));
    ptr2 += sizeof(byte);

    for(int i = 0; i < n_copies; i++) {
        uuid_t parent_id;
        memcpy(parent_id, ptr2, sizeof(uuid_t));
        ptr2 += sizeof(uuid_t);

        ptr2 += sizeof(struct timespec);

        unsigned short context_length = 0;
        memcpy(&context_length, ptr2, sizeof(unsigned short));
        ptr2 += sizeof(unsigned short);

        byte context[context_length];
        memcpy(context, ptr2, context_length);
        ptr2 += context_length;

        // parse context
        double route_cost = 0.0;
        unsigned int route_hops = 0;
        bool has_cost = false;
        bool has_hops = false;

        byte* ptr3 = context;
        unsigned int read = 0;
        while( read < context_length ) {
            char key[101];
            unsigned int key_len = strlen((char*)ptr3)+1;
            memcpy(key, ptr3, key_len);
            ptr3 += key_len;
            read += key_len;

            byte len = 0;
            memcpy(&len, ptr3, sizeof(byte));
            ptr3 += sizeof(byte);
            read += sizeof(byte);

            byte value[len];
            memcpy(value, ptr3, len);
            ptr3 += len;
            read += len;

            if(strcmp(key, "route_cost") == 0) {
                assert(len == sizeof(double));
                route_cost = *((double*)value);
                printf("context: %s -> (%u bytes, %f)\n", key, len, route_cost);
                has_cost = true;
            } else if(strcmp(key, "hops") == 0) {
                assert(len == sizeof(byte));
                route_hops = *((byte*)value);
                printf("context: %s -> (%u bytes, %u)\n", key, len, route_hops);
                has_hops = true;
            } else {
                // debug
                printf("context: %s -> (%u bytes, - )\n", key, len);
            }
        }

        RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, parent_id);
        if( neigh && RNE_isBi(neigh) && has_cost && has_hops ) { // parent is bi
            route_cost += RNE_getTxCost(neigh);
            //route_hops += 1;

            if( route_cost < min_route_cost || (route_cost == min_route_cost && route_hops < min_route_hops) || first ) {
                first = false;
                min_route_cost = route_cost;
                min_route_hops = route_hops;
                uuid_copy(min_parent_id, parent_id);
            }
        }
    }

    if(!first) {
        *found_route_cost = min_route_cost;
        *found_route_hops = min_route_hops;
        uuid_copy(found_parent, min_parent_id);
        return true;
    } else {
        return false;
    }

}

static void RecomputeRoutingTable(unsigned char* source_id, unsigned char* parent_id, double cost, unsigned int hops, RoutingNeighbors* neighbors, RoutingTable* routing_table, struct timespec* current_time) {
    list* to_update = list_init();

    RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, parent_id);
    assert(neigh);
    WLANAddr* addr = RNE_getAddr(neigh);

    RoutingTableEntry* new_entry = newRoutingTableEntry(source_id, parent_id, addr, cost, hops, current_time);

    list_add_item_to_tail(to_update, new_entry);

    RF_updateRoutingTable(routing_table, to_update, NULL, current_time);
}
