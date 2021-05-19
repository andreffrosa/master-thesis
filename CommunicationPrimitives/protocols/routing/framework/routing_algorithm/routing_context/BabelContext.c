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

typedef enum {
    BABEL_UPDT,
    BABEL_SREQ
} BabelControlMessageType;

typedef struct BabelState_ {
    bool seq_req;

    hash_table* destinations;
} BabelState;

typedef struct FeasabilityDistance_ {
    double cost;
    unsigned short seq_no;
} FeasabilityDistance;

typedef struct NeighEntry_ {
    double advertised_cost;
    unsigned int advertised_hops;
    unsigned short seq_no;
    struct timespec expiration_time;
} NeighEntry;

typedef struct DestEntry_ {
    FeasabilityDistance feasability_distance;
    hash_table* neigh_vectors;
    // struct timespec expiration_time;
} DestEntry;

static bool getParent(byte* meta_data, unsigned int meta_length, unsigned char* found_parent);

static void processUpdate(unsigned char* destination, double cost, byte hops, unsigned short seq, bool new_seq, bool new_source, RoutingNeighborsEntry* parent_entry, SourceEntry* source_entry, SourceTable* source_table, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto);

static void recomputeRoute(SourceEntry* destination, hash_table* neigh_vectors, SourceTable* source_table, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto);

static RoutingContextSendType ProcessDiscoveryEvent(YggEvent* ev, void* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, const char* proto);

static RoutingContextSendType BabelRoutingContextTriggerEvent(ModuleState* m_state, const char* proto, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {

    // TODO

    BabelState* state = (BabelState*)m_state->vars;

    if(event_type == RTE_NEIGHBORS_CHANGE) {
        YggEvent* ev = args;
        return ProcessDiscoveryEvent(ev, NULL, routing_table, neighbors, source_table, myID, current_time, proto);
    }

    else if(event_type == RTE_SOURCE_EXPIRE) {
        //SourceEntry* entry = args;

        // remove attrs from entry
        /*
        hash_table* neigh_vectors = SE_getAttr(entry, "neigh_vectors");
        if(neigh_vectors) {
            hash_table_delete(neigh_vectors);
        }

        FeasabilityDistance* feasability_distance = SE_getAttr(entry, "feasability_distance");
        free(feasability_distance);
        */

        // Remove
        /*list* to_remove = list_init();
        list_add_item_to_tail(to_remove, new_id(SE_getID(entry)));
        RF_updateRoutingTable(routing_table, NULL, to_remove, current_time);*/
    }

    else if(event_type == RTE_ANNOUNCE_TIMER) {

        if(state->seq_req) {
            state->seq_req = false;
            return SEND_INC;
        } else {
            return SEND_NO_INC;
        }
    }

    return NO_SEND;
}

static void BabelRoutingContextCreateMsg(ModuleState* m_state, const char* proto, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info) {

    BabelState* state = (BabelState*)m_state->vars;

    // TODO: fazer os requests

    if(event_type == RTE_ANNOUNCE_TIMER) {
        byte type = BABEL_UPDT;
        YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

        //byte b = true;
        //YggMessage_addPayload(msg, (char*)&b, sizeof(byte));

        // TODO: Verify if all routes are still valid and recompute if necessary

        byte amount = RT_size(routing_table); // TODO: include infinites
        YggMessage_addPayload(msg, (char*)&amount, sizeof(byte));

        // TODO
        void* iterator = NULL;
        RoutingTableEntry* rte = NULL;
        while( (rte = RT_nextRoute(routing_table, &iterator)) ) {

            DestEntry* de = hash_table_find_value(state->destinations, RTE_getDestinationID(rte));
            assert(de);

            NeighEntry* ne = hash_table_find_value(de->neigh_vectors, RTE_getNextHopID(rte));
            assert(ne);

            assert(compare_timespec(&ne->expiration_time, current_time) > 0);

            YggMessage_addPayload(msg, (char*)RTE_getDestinationID(rte), sizeof(uuid_t));

            YggMessage_addPayload(msg, (char*)&ne->seq_no, sizeof(unsigned short));

            double cost = RTE_getCost(rte);
            YggMessage_addPayload(msg, (char*)&cost, sizeof(double));

            byte hops = RTE_getHops(rte);
            YggMessage_addPayload(msg, (char*)&hops, sizeof(byte));
        }

        // TODO: include retracted destinations
        // double cost = INFINITY;
        // byte hops = 255;

    }
}

// #define MSG_CONTROL_MESSAGE 1

static RoutingContextSendType BabelRoutingContextProcessMsg(ModuleState* m_state, const char* proto, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, unsigned short src_proto, byte* meta_data, unsigned int meta_length, bool new_seq, bool new_source, void* f_state) {

    uuid_t parent_id;
    getParent(meta_data, meta_length, parent_id);

    RoutingNeighborsEntry* parent_entry = RN_getNeighbor(neighbors, parent_id);
    if( parent_entry && RNE_isBi(parent_entry)) { // parent is bi

        void* ptr = payload;

        byte type = 0;
        memcpy(&type, ptr, sizeof(byte));
        ptr += sizeof(byte);

        byte b = 0;
        memcpy(&b, ptr, sizeof(byte));
        ptr += sizeof(byte);

        if(b) {
            // process update
            processUpdate(SE_getID(source_entry), 0, 0, SE_getSEQ(source_entry), new_seq, new_source, parent_entry, source_entry, source_table, routing_table, neighbors, current_time, proto);

            byte amount = 0;
            memcpy(&amount, ptr, sizeof(byte));
            ptr += sizeof(byte);

            for(int i = 0; i < amount; i++) {
                uuid_t id;
                memcpy(id, ptr, sizeof(uuid_t));
                ptr += sizeof(uuid_t);

                unsigned short seq = 0;
                memcpy(&seq, ptr, sizeof(unsigned short));
                ptr += sizeof(unsigned short);

                double cost = 0;
                memcpy(&cost, ptr, sizeof(double));
                ptr += sizeof(double);

                byte hops = 0;
                memcpy(&hops, ptr, sizeof(byte));
                ptr += sizeof(byte);

                byte period_s = SE_getPeriod(source_entry);

                YggMessage m;
                YggMessage_initBcast(&m, 0);

                RoutingControlHeader header = {0};
                initRoutingControlHeader(&header, seq, period_s);

                // Insert Message Type
                //byte m_type = MSG_CONTROL_MESSAGE;
                //int add_result = YggMessage_addPayload(&m, (char*) &m_type, sizeof(byte));
                //assert(add_result != FAILED);

                // Insert Header
                int add_result = YggMessage_addPayload(&m, (char*) &header, sizeof(RoutingControlHeader));
                assert(add_result != FAILED);

                byte type = BABEL_UPDT;
                YggMessage_addPayload(&m, (char*)&type, sizeof(byte));

                byte b = false;
                YggMessage_addPayload(&m, (char*)&b, sizeof(byte));

                YggMessage_addPayload(&m, (char*)&cost, sizeof(double));
                YggMessage_addPayload(&m, (char*)&hops, sizeof(byte));

                RF_uponNewControlMessage2(f_state, &m, id, src_proto, meta_data, meta_length);
            }
        } else {
            double cost = 0;
            memcpy(&cost, ptr, sizeof(double));
            ptr += sizeof(double);

            byte hops = 0;
            memcpy(&hops, ptr, sizeof(byte));
            ptr += sizeof(byte);

            // process update
            processUpdate(SE_getID(source_entry), cost, hops, SE_getSEQ(source_entry), new_seq, new_source, parent_entry, source_entry, source_table, routing_table, neighbors, current_time, proto);
        }
    }

    return NO_SEND;
}

RoutingContext* BabelRoutingContext() {

    BabelState* state = malloc(sizeof(BabelState));
    state->seq_req = false;

    return newRoutingContext(
        "BABEL",
        NULL,
        state,
        NULL, // &BabelRoutingContextInit,
        &BabelRoutingContextTriggerEvent,
        &BabelRoutingContextCreateMsg,
        &BabelRoutingContextProcessMsg,
        NULL
    );

}

static RoutingContextSendType ProcessDiscoveryEvent(YggEvent* ev, void* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, const char* proto) {
    assert(ev);

    unsigned short ev_id = ev->notification_id;

    if(ev_id == UPDATE_NEIGHBOR) {

        SourceEntry* se = NULL;
        void* iterator = NULL;
        while( (se = ST_nexEntry(source_table, &iterator)) ) {
            hash_table* neigh_vectors = SE_getAttr(se, "neigh_vectors");

            if(neigh_vectors) {
                // Update routing table, if necessary
                recomputeRoute(se, neigh_vectors, source_table, routing_table, neighbors, current_time, proto);
            }
        }
    }


    // enviar triggered update quando vizinhança muda
    return SEND_NO_INC;


    /*bool process = ev_id == NEW_NEIGHBOR || ev_id == UPDATE_NEIGHBOR || ev_id == LOST_NEIGHBOR;
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
                addRoute(id, id, cost, 1, neighbors, routing_table, current_time, proto);
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
    }*/
}


static bool isFeasible(FeasabilityDistance* feasability_distance, double cost, unsigned short seq_no) {

    if(cost == INFINITY) {
        return true;
    } else if(feasability_distance == NULL) {
        return true;
    } else {
        return feasability_distance->seq_no < seq_no || (feasability_distance->seq_no == seq_no);
    }
}

extern MyLogger* routing_logger;

static void processUpdate(unsigned char* destination, double cost, byte hops, unsigned short seq_no, bool new_seq, bool new_source, RoutingNeighborsEntry* parent_entry, SourceEntry* source_entry, SourceTable* source_table, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto) {

    char id_str1[UUID_STR_LEN];
    uuid_unparse(destination, id_str1);

    char id_str2[UUID_STR_LEN];
    uuid_unparse(RNE_getID(parent_entry), id_str2);

    //double route_cost = cost + RNE_getRxCost(parent_entry);
    //byte route_hops = hops + 1;

    char str[200];
    sprintf(str, "to %s from %s SEQ=%hu COST=%f HOPS=%d", id_str1, id_str2, seq_no, cost, hops);

    my_logger_write(routing_logger, "BABEL", "UPDATE", str);

    FeasabilityDistance* feasability_distance = SE_getAttr(source_entry, "feasability_distance");
    bool is_feasible = isFeasible(feasability_distance, cost, seq_no);

    hash_table* neigh_vectors = SE_getAttr(source_entry, "neigh_vectors");
    if(neigh_vectors == NULL) {
        neigh_vectors = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
        SE_setAttr(source_entry, new_str("neigh_vectors"), neigh_vectors);
    }

    // Route Acquisition
    NeighEntry* neigh_vector = hash_table_find_value(neigh_vectors, RNE_getID(parent_entry));
    if(neigh_vector) {
        RoutingTableEntry* rte = RT_findEntry(routing_table, destination);

        bool selected = false;
        if(rte) {
            selected = uuid_compare(RNE_getID(parent_entry), RTE_getNextHopID(rte)) == 0;
        }

        if(!selected || is_feasible) {
            neigh_vector->seq_no = seq_no;
            neigh_vector->advertised_cost = cost;
            neigh_vector->advertised_hops = hops;
        }

        /*if(selected && !is_feasible) {
            // remove
        }*/

    } else {
        if(is_feasible && cost != INFINITY) {
            neigh_vector = malloc(sizeof(NeighEntry));
            neigh_vector->seq_no = SE_getSEQ(source_entry);
            neigh_vector->advertised_cost = cost;
            neigh_vector->advertised_hops = hops;

            hash_table_insert(neigh_vectors, new_id(RNE_getID(parent_entry)), neigh_vector);
        }
    }

    // Update routing table, if necessary
    recomputeRoute(source_entry, neigh_vectors, source_table, routing_table, neighbors, current_time, proto);
}

static bool getParent(byte* meta_data, unsigned int meta_length, unsigned char* found_parent) {

    //uuid_t min_parent_id = {0};
    //bool first = true;

    byte* ptr2 = meta_data;

    uuid_t source_id;
    memcpy(source_id, ptr2, sizeof(uuid_t));
    ptr2 += sizeof(uuid_t);

    byte n_copies = 0;
    memcpy(&n_copies, ptr2, sizeof(byte));
    ptr2 += sizeof(byte);

    //for(int i = 0; i < n_copies; i++) {
    uuid_t parent_id;
    memcpy(parent_id, ptr2, sizeof(uuid_t));
    ptr2 += sizeof(uuid_t);

    uuid_copy(found_parent, parent_id);
    return true;

    /*ptr2 += sizeof(struct timespec);

    unsigned short context_length = 0;
    memcpy(&context_length, ptr2, sizeof(unsigned short));
    ptr2 += sizeof(unsigned short);

    //byte context[context_length];
    //memcpy(context, ptr2, context_length);
    ptr2 += context_length;*/
    //}

    /*if(!first) {
    *found_route_cost = min_route_cost;
    *found_route_hops = min_route_hops;
    uuid_copy(found_parent, min_parent_id);
    return true;
} else {
return false;
}*/

}

static void recomputeRoute(SourceEntry* destination, hash_table* neigh_vectors, SourceTable* source_table, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto) {

    double* feasability_distance = SE_getAttr(destination, "feasability_distance");

    hash_table_item* best_neigh = NULL;
    double best_cost = INFINITY;
    byte best_hops = 255;
    hash_table_item* current_neigh = NULL;
    void* iterator = NULL;
    while( (current_neigh = hash_table_iterator_next(neigh_vectors, &iterator)) ) {
        unsigned char* neigh_id = current_neigh->key;
        NeighEntry* neigh_vector = current_neigh->value;

        RoutingNeighborsEntry* rte = RN_getNeighbor(neighbors, neigh_id);

        if(rte) {
            double route_cost = neigh_vector->advertised_cost + RNE_getRxCost(rte);
            byte route_hops = neigh_vector->advertised_hops + 1;

            //bool new_seq_ = uuid_compare(neigh_id, RNE_getID(parent_entry)) == 0 ? new_seq : false;
            bool new_seq_ = compare_seq(neigh_vector->seq_no, SE_getSEQ(destination), true) > 0;

            if(RNE_isBi(rte) && neigh_vector->advertised_cost != INFINITY && isFeasible(feasability_distance, neigh_vector->advertised_cost, new_seq_, feasability_distance == NULL)) {
                bool select = route_cost < best_cost ? true : (route_cost == best_cost ? route_hops < best_hops : false);
                if(select) {
                    best_neigh = current_neigh;
                    best_cost = route_cost;
                    best_hops = route_hops;
                }
            }

        } else {
            assert(false);
        }
    }

    if(best_neigh) {
        // Update

        RoutingNeighborsEntry* best_neigh_entry = RN_getNeighbor(neighbors, best_neigh->key);
        assert(best_neigh_entry);

        list* to_update = list_init();

        WLANAddr* addr = RNE_getAddr(best_neigh_entry);
        RoutingTableEntry* new_entry = newRoutingTableEntry(SE_getID(destination), RNE_getID(best_neigh_entry), addr, best_cost, best_hops, current_time, proto);

        list_add_item_to_tail(to_update, new_entry);

        RF_updateRoutingTable(routing_table, to_update, NULL, current_time);

        if(feasability_distance) {
            *feasability_distance = best_cost;
        } else {
            SE_setAttr(destination, new_str("feasability_distance"), new_double(best_cost));
        }

    } else {
        // Remove if exist
    }

}
