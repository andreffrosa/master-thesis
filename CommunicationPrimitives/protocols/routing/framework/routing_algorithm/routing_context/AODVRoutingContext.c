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

#include "protocols/discovery/framework/framework.h"
#include "protocols/routing/framework/framework.h"

#include <assert.h>

typedef enum {
    AODV_RREQ,
    AODV_RREP,
    AODV_RERR
} AODVControlMessageType;

static bool getBestBiParent(RoutingNeighbors* neighbors, byte* meta_data, unsigned int meta_length, unsigned char* found_parent, double* found_route_cost, unsigned int* found_route_hops);

static void ProcessDiscoveryEvent(YggEvent* ev, void* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time);

static void RecomputeRoutingTable(unsigned char* source_id, unsigned char* parent_id, double cost, unsigned int hops, RoutingNeighbors* neighbors, RoutingTable* routing_table, struct timespec* current_time);

/*static void AODVRoutingContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time) {

}*/

static RoutingContextSendType AODVRoutingContextTriggerEvent(ModuleState* m_state, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {

    if(event_type == RTE_ROUTE_NOT_FOUND) {
        //printf("SENDING RREQ\n");
        return SEND_INC;
    } else if(event_type == RTE_NEIGHBORS_CHANGE) {
        YggEvent* ev = args;
        ProcessDiscoveryEvent(ev, NULL, routing_table, neighbors, source_table, myID, current_time);
    }

    return NO_SEND;
}

static void AODVRoutingContextCreateMsg(ModuleState* m_state, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info) {

    if( event_type == RTE_ROUTE_NOT_FOUND ) {
        assert(info);
        RoutingHeader* header2 = info;

        byte type = 0;

        // Send RREQ
        if( uuid_compare(header2->source_id, myID) == 0 ) {
            type = AODV_RREQ;

            YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

            YggMessage_addPayload(msg, (char*)header2->destination_id, sizeof(uuid_t));

            char str[UUID_STR_LEN];
            uuid_unparse(header2->destination_id, str);
            printf("GENERATING REQ to %s\n", str);
        }
        // Send RERR
        else {
            type = AODV_RERR;

            YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

            //YggMessage_addPayload(msg, (char*)header2->source_id, sizeof(uuid_t));

            YggMessage_addPayload(msg, (char*)header2->destination_id, sizeof(uuid_t));

            /*
            YggMessage_addPayload(msg, (char*)myID, sizeof(uuid_t));

            YggMessage_addPayload(msg, (char*)header2->prev_hop_id, sizeof(uuid_t));
            */

            char str[UUID_STR_LEN];
            uuid_unparse(header2->destination_id, str);
            printf("GENERATING RERR to remove %s\n", str);
        }

    } else if(event_type == RTE_CONTROL_MESSAGE) {
        assert(info);
        SourceEntry* entry = ((void**)info)[0];
        byte* payload = ((void**)info)[1];
        //unsigned short length = *((unsigned short*)((void**)info)[2]);

        byte* ptr = payload;

        byte msg_type = 0;
        memcpy(&msg_type, ptr, sizeof(byte));
        ptr += sizeof(byte);

        if(msg_type == AODV_RREP || msg_type == AODV_RREQ) {
            byte type = AODV_RREP;
            YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

            double route_cost = 0.0;
            byte route_hops = 0;

            if( msg_type == AODV_RREP ) {
                RoutingTableEntry* rt_entry = RT_findEntry(routing_table, SE_getID(entry)); // The source of the rrep
                route_cost = RTE_getCost(rt_entry);
                route_hops = RTE_getHops(rt_entry);
            }

            YggMessage_addPayload(msg, (char*)&route_cost, sizeof(double));
            YggMessage_addPayload(msg, (char*)&route_hops, sizeof(byte));

            /*
            uuid_t destination_id;
            double route_cost = 0.0;
            byte route_hops = 0;

            // Is reply?
            if( uuid_compare(myID, header->source_id) == 0 ) {
                uuid_copy(destination_id, SE_getID(entry));

            } else {
                byte* ptr = payload + sizeof(byte);

                memcpy(destination_id, ptr, sizeof(uuid_t));
                //ptr += sizeof(uuid_t);

                RoutingTableEntry* rt_entry = RT_findEntry(routing_table, SE_getID(entry)); // The source of the rrep
                route_cost = RTE_getCost(rt_entry);
                route_hops = RTE_getHops(rt_entry);
            }

            YggMessage_addPayload(msg, (char*)destination_id, sizeof(uuid_t));

            YggMessage_addPayload(msg, (char*)myID, sizeof(uuid_t));

            RoutingTableEntry* rt_entry = RT_findEntry(routing_table, destination_id); // The source of the rreq
            assert(rt_entry);
            YggMessage_addPayload(msg, (char*)RTE_getNextHopID(rt_entry) , sizeof(uuid_t));

            YggMessage_addPayload(msg, (char*)&route_cost, sizeof(double));
            YggMessage_addPayload(msg, (char*)&route_hops, sizeof(byte));
*/
        } else if(msg_type == AODV_RERR) {
            byte type = AODV_RERR;

            YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

            byte* ptr = payload + sizeof(byte);

            /*
            uuid_t destination_id;
            memcpy(destination_id, ptr, sizeof(uuid_t));
            ptr += sizeof(uuid_t);

            YggMessage_addPayload(msg, (char*)destination_id, sizeof(uuid_t));
            */

            uuid_t dest_to_remove_id;
            memcpy(dest_to_remove_id, ptr, sizeof(uuid_t));
            ptr += sizeof(uuid_t);
            YggMessage_addPayload(msg, (char*)dest_to_remove_id, sizeof(uuid_t));

            /*YggMessage_addPayload(msg, (char*)myID, sizeof(uuid_t));

            RoutingTableEntry* rt_entry = RT_findEntry(routing_table, destination_id);
            assert(rt_entry);

            YggMessage_addPayload(msg, (char*)RTE_getNextHopID(rt_entry), sizeof(uuid_t));
            */
        }
    }
}

static RoutingContextSendType AODVRoutingContextProcessMsg(ModuleState* m_state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, unsigned short src_proto, byte* meta_data, unsigned int meta_length, bool* forward) {

    void* ptr = payload;

    byte type = 0;
    memcpy(&type, ptr, sizeof(byte));
    ptr += sizeof(byte);

    if( type == AODV_RREQ ) {
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

                printf("I'm the wanted destination! SEND RREP\n");

                return SEND_INC; // SEND RREP
            }
        }
    } else if( type == AODV_RREP ) {

        uuid_t destination_id;
        uuid_t prev_hop_id;

        if(src_proto == ROUTING_FRAMEWORK_PROTO_ID) {
            RoutingHeader* header = (RoutingHeader*)meta_data;

            uuid_copy(destination_id, header->destination_id);
            uuid_copy(prev_hop_id, header->prev_hop_id);
        } else {
            assert(false);
        }

        //uuid_t next_hop_id;

        double route_cost = 0.0;
        byte route_hops = 0;
        
        memcpy(&route_cost, ptr, sizeof(double));
        ptr += sizeof(double);

        memcpy(&route_hops, ptr, sizeof(byte));
        ptr += sizeof(byte);


        char str[UUID_STR_LEN];
        uuid_unparse(SE_getID(source_entry), str);
        printf("RECEIVED RREP with route to %s\n", str);

        RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, prev_hop_id);
        if(neigh && RNE_isBi(neigh)) {
            route_cost += RNE_getTxCost(neigh);
            route_hops += 1;

            // Update routing table
            RecomputeRoutingTable(SE_getID(source_entry), prev_hop_id, route_cost, route_hops, neighbors, routing_table, current_time);

            /*
            // Forward
            if( uuid_compare(myID, next_hop_id) == 0 && uuid_compare(myID, destination_id) != 0 ) {
                *forward = true;
                return SEND_NO_INC;
            }
            */
        }
    }
    else if( type == AODV_RERR ) {

        uuid_t dest_to_remove_id;
        uuid_t prev_hop_id;
        //uuid_t next_hop_id;

        if(src_proto == ROUTING_FRAMEWORK_PROTO_ID) {
            RoutingHeader* header = (RoutingHeader*)meta_data;

            uuid_copy(prev_hop_id, header->prev_hop_id);
        } else {
            assert(false);
        }

        memcpy(dest_to_remove_id, ptr, sizeof(uuid_t));
        ptr += sizeof(uuid_t);

        /*
        memcpy(prev_hop_id, ptr, sizeof(uuid_t));
        ptr += sizeof(uuid_t);

        memcpy(next_hop_id, ptr, sizeof(uuid_t));
        ptr += sizeof(uuid_t);
        */

        char str[UUID_STR_LEN];
        uuid_unparse(dest_to_remove_id, str);
        printf("RECEIVED RERR to %s\n", str);

        // Update Routing Table
        list* to_remove = list_init();

        list_add_item_to_tail(to_remove, new_id(dest_to_remove_id));

        RF_updateRoutingTable(routing_table, NULL, to_remove, current_time);

        /*
        // Forward
        if( uuid_compare(myID, next_hop_id) == 0 && uuid_compare(myID, destination_id) != 0 ) {
            *forward = true;
            return SEND_NO_INC;
        }
        */

    }

    return NO_SEND;
}

//typedef void (*rc_destroy)(ModuleState* m_state);

RoutingContext* AODVRoutingContext() {

    return newRoutingContext(
        NULL,
        NULL,
        NULL, // &AODVRoutingContextInit,
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


static void ProcessDiscoveryEvent(YggEvent* ev, void* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {
    assert(ev);

    unsigned short ev_id = ev->notification_id;

    bool process = ev_id == NEW_NEIGHBOR || ev_id == UPDATE_NEIGHBOR || ev_id == LOST_NEIGHBOR;
    if(process) {
        unsigned short read = 0;
        unsigned char* ptr = ev->payload;

        unsigned short length = 0;
        memcpy(&length, ptr, sizeof(unsigned short));
        ptr += sizeof(unsigned short);
        read += sizeof(unsigned short);

        YggEvent main_ev = {0};
        YggEvent_init(&main_ev, DISCOVERY_FRAMEWORK_PROTO_ID, 0);
        YggEvent_addPayload(&main_ev, ptr, length);
        ptr += length;
        read += length;

        unsigned char* ptr2 = NULL;

        uuid_t id;
        ptr2 = YggEvent_readPayload(&main_ev, ptr2, id, sizeof(uuid_t));

        bool remove = false;

        if(ev_id == NEW_NEIGHBOR || ev_id == UPDATE_NEIGHBOR) {
            RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, id);
            assert(neigh);

            if( RNE_isBi(neigh) ) {
                double cost = RNE_getTxCost(neigh);
                RecomputeRoutingTable(id, id, cost, 1, neighbors, routing_table, current_time);
            } else {
                remove = true;
            }
        } else {
            remove = true;
        }

        if(remove) {
            list* to_remove = list_init();

            //list_add_item_to_tail(to_remove, new_id(id));

            void* iterator = NULL;
            RoutingTableEntry* current_route = NULL;
            while( (current_route = RT_nextRoute(routing_table, &iterator)) ) {
                if( uuid_compare(RTE_getNextHopID(current_route), id) == 0) {
                    list_add_item_to_tail(to_remove, new_id(RTE_getDestinationID(current_route)));
                }
            }

            RF_updateRoutingTable(routing_table, NULL, to_remove, current_time);
        }
    }
}
