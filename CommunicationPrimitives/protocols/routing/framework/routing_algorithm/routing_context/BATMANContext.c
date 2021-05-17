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

#include "utility/my_string.h"
#include "utility/my_time.h"

#include "utility/seq.h"

#include <assert.h>

/*
typedef struct BATMANState_ {
} BATMANState;
*/

/*typedef struct OGEntry_ {
    hash_table* neigh_info;
} OGEntry;*/

typedef struct NeighEntry_ {
    uuid_t id;
    list* sliding_window;
    //unsigned int packet_count;
    struct timespec last_valid_time;
    //unsigned short last_ttl;
    double cost;
    unsigned int hops;
} NeighEntry;


static bool getFirstBiParent(RoutingNeighbors* neighbors, byte* meta_data, unsigned int meta_length, unsigned char* found_parent, double* found_route_cost, unsigned int* found_route_hops);

static void processOGM(RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceEntry* source_entry, unsigned char* found_parent, double found_route_cost, unsigned int found_route_hops, unsigned char* myID, struct timespec* current_time, const char* proto);

static RoutingContextSendType ProcessDiscoveryEvent(YggEvent* ev, void* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, const char* proto);

static RoutingContextSendType BATMANRoutingContextTriggerEvent(ModuleState* m_state, const char* proto, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {

    if(event_type == RTE_NEIGHBORS_CHANGE) {
        YggEvent* ev = args;
        return ProcessDiscoveryEvent(ev, NULL, routing_table, neighbors, source_table, myID, current_time, proto);
    }

    else if(event_type == RTE_SOURCE_EXPIRE) {
        SourceEntry* entry = args;

        // remove attrs from entry
        hash_table* neigh_info = SE_remAttr(entry, "neigh_info");
        if(neigh_info) {
            hash_table_item* hit = NULL;
            void* iterator = NULL;
            while ( (hit = hash_table_iterator_next(neigh_info, &iterator)) ) {
                NeighEntry* e = hit->value;
                list_delete(e->sliding_window);
                free(e);
                hit->value = NULL;
            }
            hash_table_delete(neigh_info);
        }

        // Remove
        list* to_remove = list_init();
        list_add_item_to_tail(to_remove, new_id(SE_getID(entry)));
        RF_updateRoutingTable(routing_table, NULL, to_remove, current_time);
    }

    else if(event_type == RTE_ANNOUNCE_TIMER) {
        return SEND_INC;
    }

    return NO_SEND;
}

static void BATMANRoutingContextCreateMsg(ModuleState* m_state, const char* proto, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info) {
    // Do nothing
}

static RoutingContextSendType BATMANRoutingContextProcessMsg(ModuleState* m_state, const char* proto, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, unsigned short src_proto, byte* meta_data, unsigned int meta_length) {

    uuid_t found_parent = {0};
    double found_route_cost = 0.0;
    unsigned int found_route_hops = 0;
    bool found = getFirstBiParent(neighbors, meta_data, meta_length, found_parent, &found_route_cost, &found_route_hops);
    if( found ) {
        processOGM(routing_table, neighbors, source_entry, found_parent, found_route_cost, found_route_hops, myID, current_time, proto);
    }

    return NO_SEND;
}

RoutingContext* BATMANRoutingContext() {

    return newRoutingContext(
        "BATMAN",
        NULL,
        NULL,
        NULL, // &BATMANRoutingContextInit,
        &BATMANRoutingContextTriggerEvent,
        &BATMANRoutingContextCreateMsg,
        &BATMANRoutingContextProcessMsg,
        NULL
    );

}

static bool getFirstBiParent(RoutingNeighbors* neighbors, byte* meta_data, unsigned int meta_length, unsigned char* found_parent, double* found_route_cost, unsigned int* found_route_hops) {

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
                //printf("context: %s -> (%u bytes, %f)\n", key, len, route_cost);
                has_cost = true;
            } else if(strcmp(key, "hops") == 0) {
                assert(len == sizeof(byte));
                route_hops = *((byte*)value);
                //printf("context: %s -> (%u bytes, %u)\n", key, len, route_hops);
                has_hops = true;
            } else {
                // debug
                //printf("context: %s -> (%u bytes, - )\n", key, len);
            }
        }

        RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, parent_id);
        if( neigh && RNE_isBi(neigh) && has_cost && has_hops ) { // parent is bi
            route_cost += RNE_getTxCost(neigh);
            //route_hops += 1;

            //if( route_cost < min_route_cost || (route_cost == min_route_cost && route_hops < min_route_hops) || first ) {
                first = false;
                min_route_cost = route_cost;
                min_route_hops = route_hops;
                uuid_copy(min_parent_id, parent_id);
            //}
        }

        if(!first)
            break;
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

static void processOGM(RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceEntry* source_entry, unsigned char* found_parent, double found_route_cost, unsigned int found_route_hops, unsigned char* myID, struct timespec* current_time, const char* proto) {

    /*
    char str1[UUID_STR_LEN];
    uuid_unparse(SE_getID(source_entry), str1);

    char str2[UUID_STR_LEN];
    uuid_unparse(found_parent, str2);

    printf("Received OGM from %s through %s\n", str1, str2);
    */

    hash_table* neigh_info = SE_getAttr(source_entry, "neigh_info");
    if(neigh_info == NULL) {
        neigh_info = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    }

    NeighEntry* e = (NeighEntry*)hash_table_find_value(neigh_info, found_parent);
    if(e == NULL) {
        e = malloc(sizeof(NeighEntry));

        e->sliding_window = list_init();
        //e->packet_count = 0;
        //e->last_ttl;

        e->hops = found_route_hops;
        e->cost = found_route_cost;
        uuid_copy(e->id, found_parent);

        hash_table_insert(neigh_info, new_id(found_parent), e);
    }

    list_add_item_to_tail(e->sliding_window, new_short(SE_getSEQ(source_entry)));

    // TODO: params
    unsigned int window_size = 128;
    bool ignore_zero = true;

    while(  compare_seq(*((unsigned short*)e->sliding_window->head->data), sub_seq(SE_getSEQ(source_entry), window_size, ignore_zero), ignore_zero) < 0 ) {
        unsigned short* x = list_remove_head(e->sliding_window);
        free(x);
    }

    copy_timespec(&e->last_valid_time, current_time);

    NeighEntry* best = NULL;

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(neigh_info, &iterator)) ) {
        NeighEntry* x = hit->value;

        if(best == NULL || best->sliding_window->size < x->sliding_window->size) {
            best = x;
        }
    }

    list* to_update = list_init();

    RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, best->id);
    assert(neigh);
    WLANAddr* addr = RNE_getAddr(neigh);
    RoutingTableEntry* new_entry = newRoutingTableEntry(SE_getID(source_entry), best->id, addr, best->cost, best->hops, current_time, proto);

    list_add_item_to_tail(to_update, new_entry);

    RF_updateRoutingTable(routing_table, to_update, NULL, current_time);
}


static RoutingContextSendType ProcessDiscoveryEvent(YggEvent* ev, void* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, const char* proto) {
    assert(ev);

    unsigned short ev_id = ev->notification_id;

    bool process = /*ev_id == NEW_NEIGHBOR || ev_id == UPDATE_NEIGHBOR ||*/ ev_id == LOST_NEIGHBOR;
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

        list* to_remove = list_init();

        void* iterator = NULL;
        SourceEntry* se = NULL;
        while( (se = ST_nexEntry(source_table, &iterator)) ) {
            hash_table* neigh_info = SE_getAttr(se, "neigh_info");
            if(neigh_info) {
                NeighEntry* ne = hash_table_remove(neigh_info, id);
                if(ne) {
                    list_delete(ne->sliding_window);
                    free(ne);

                    // Atualizar a routing table
                    RoutingTableEntry* re = RT_findEntry(routing_table, SE_getID(se));
                    if(re) {
                        if( uuid_compare(RTE_getNextHopID(re), id) == 0) {
                            // Remove
                            list_add_item_to_tail(to_remove, new_id(SE_getID(se)));
                        }
                    }
                }
            }
        }

        RF_updateRoutingTable(routing_table, NULL, to_remove, current_time);
    }

    return NO_SEND;
}
