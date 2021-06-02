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
#include "utility/my_math.h"


#include "utility/seq.h"

#include <assert.h>

extern MyLogger* routing_logger;

typedef enum {
    BABEL_UPDT = 0,
    BABEL_SREQ
} BabelControlMessageType;

typedef struct BabelState_ {
    bool seq_req;

    hash_table* destinations;

    list* starvation;
    bool trigger_update;
    hash_table* pending_requests;
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
    FeasabilityDistance* feasability_distance;
    hash_table* neigh_vectors;
    struct timespec expiration_time;
    bool retracted;
} DestEntry;

typedef struct PendingReq_ {
    unsigned short seq_no;
    struct timespec expiration_time;
} PendingReq;

static bool getParent(byte* meta_data, unsigned int meta_length, unsigned char* found_parent);

static void processUpdate(BabelState* state, unsigned char* destination_id, double cost, byte hops, unsigned short seq_no, RoutingNeighborsEntry* parent_entry, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto, list* to_update, list* to_remove);

static void recomputeAllRoutes(BabelState* state, hash_table* destinations, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto);

static void recomputeRoute(BabelState* state, unsigned char* destination_id, DestEntry* de, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto, list* to_update, list* to_remove);

static RoutingContextSendType ProcessDiscoveryEvent(YggEvent* ev, BabelState* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, const char* proto);

static RoutingContextSendType BabelRoutingContextTriggerEvent(ModuleState* m_state, const char* proto, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {

    BabelState* state = (BabelState*)m_state->vars;

    if(event_type == RTE_NEIGHBORS_CHANGE) {
        YggEvent* ev = args;
        return ProcessDiscoveryEvent(ev, state, routing_table, neighbors, source_table, myID, current_time, proto);
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

static RoutingContextSendType BabelRoutingContextCreateMsg(ModuleState* m_state, const char* proto, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info) {

    BabelState* state = (BabelState*)m_state->vars;

    if(event_type == RTE_ANNOUNCE_TIMER || state->trigger_update /*|| event_type == RTE_NEIGHBORS_CHANGE*/) {

        byte type = BABEL_UPDT;
        YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

        // Verify if all routes are still valid and recompute if necessary
        recomputeAllRoutes(state, state->destinations, routing_table, neighbors, current_time, proto);

        state->trigger_update = false;

        byte amount = 0; //state->destinations->n_items;
        byte* amount_ptr = (byte*)(msg->data + msg->dataLen);
        YggMessage_addPayload(msg, (char*)&amount, sizeof(byte));

        void* iterator = NULL;
        hash_table_item* hit = NULL;
        while( (hit = hash_table_iterator_next(state->destinations, &iterator)) ) {
            unsigned char* destination_id = hit->key;
            DestEntry* de = hit->value;

            //if(de->feasability_distance) {

                // Not expired
                if(true) {
                    if(de->retracted && de->feasability_distance) {
                        if(compare_timespec(&de->expiration_time, current_time) > 0) {
                            YggMessage_addPayload(msg, (char*)destination_id, sizeof(uuid_t));

                            YggMessage_addPayload(msg, (char*)&de->feasability_distance->seq_no, sizeof(unsigned short));

                            double cost = INFINITY;
                            YggMessage_addPayload(msg, (char*)&cost, sizeof(double));

                            byte hops = 255;
                            YggMessage_addPayload(msg, (char*)&hops, sizeof(byte));

                            amount++;
                        }
                    } else {
                        RoutingTableEntry* rte = RT_findEntry(routing_table, destination_id);
                        //assert(rte);

                        if(rte) {
                            NeighEntry* ne = hash_table_find_value(de->neigh_vectors, RTE_getNextHopID(rte));
                            assert(ne);

                            YggMessage_addPayload(msg, (char*)RTE_getDestinationID(rte), sizeof(uuid_t));

                            YggMessage_addPayload(msg, (char*)&ne->seq_no, sizeof(unsigned short));

                            double cost = RTE_getCost(rte);
                            YggMessage_addPayload(msg, (char*)&cost, sizeof(double));

                            byte hops = RTE_getHops(rte);
                            YggMessage_addPayload(msg, (char*)&hops, sizeof(byte));

                            // Update feasability distance
                            if(de->feasability_distance == NULL) {
                                de->feasability_distance = malloc(sizeof(FeasabilityDistance));

                                de->feasability_distance->seq_no = ne->seq_no;
                                de->feasability_distance->cost = cost;
                            } else {
                                int seq_cmp = compare_seq(ne->seq_no, de->feasability_distance->seq_no, true);
                                if(seq_cmp > 0) {
                                    de->feasability_distance->seq_no = ne->seq_no;
                                    de->feasability_distance->cost = cost;
                                } else if(seq_cmp == 0) {
                                    de->feasability_distance->cost = dMin(de->feasability_distance->cost, cost);
                                } else {
                                    assert(false); // unfeasible
                                }
                            }

                            struct timespec aux;
                            milli_to_timespec(&aux, 5*5*1000);
                            add_timespec(&de->expiration_time, current_time, &aux);

                            amount++;
                        }
                    }
                } else {
                    char id_str[UUID_STR_LEN];
                    uuid_unparse(destination_id, id_str);

                    my_logger_write(routing_logger, "BABEL", "DEL DEST", id_str);

                    DestEntry* x = hash_table_remove(state->destinations, destination_id);
                    assert(x == de);

                    void* iterator = NULL;
                    hash_table_item* hit = NULL;
                    while( (hit = hash_table_iterator_next(x->neigh_vectors, &iterator)) ) {
                        unsigned char* neigh_id = hit->key;
                        NeighEntry* ne = hit->value;

                        NeighEntry* y = hash_table_remove(x->neigh_vectors, neigh_id);
                        assert(y == ne);

                        free(y);
                    }
                    hash_table_delete(x->neigh_vectors);

                    if(x->feasability_distance) {
                        free(x->feasability_distance);
                    }

                    free(x);
                }
            //}
        }

        *amount_ptr = amount;

        /*byte amount = RT_size(routing_table); // TODO: include infinites
        YggMessage_addPayload(msg, (char*)&amount, sizeof(byte));

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

            // Update feasability distance
            if(de->feasability_distance == NULL) {
                de->feasability_distance = malloc(sizeof(FeasabilityDistance));
            }

            de->feasability_distance->seq_no = ne->seq_no;
            de->feasability_distance->cost = cost;
        }*/

        // TODO: include retracted destinations
        // double cost = INFINITY;
        // byte hops = 255;

    } else if(state->starvation->size > 0) {

        unsigned char* destination_id = list_remove_head(state->starvation);

        DestEntry* de = hash_table_find_value(state->destinations, destination_id);

        if(de) {
            if(de->feasability_distance) {
                //char str[200];
                //sprintf(str, "to %s from %s SEQ=%hu COST=%f HOPS=%d", id_str1, id_str2, seq_no, cost, hops);

                char id_str[UUID_STR_LEN];
                uuid_unparse(destination_id, id_str);

                my_logger_write(routing_logger, "BABEL", "SEQ REQ", id_str);

                byte type = BABEL_SREQ;
                YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

                YggMessage_addPayload(msg, (char*)destination_id, sizeof(uuid_t));

                unsigned short seq_no = inc_seq(de->feasability_distance->seq_no, true);
                YggMessage_addPayload(msg, (char*)&seq_no, sizeof(unsigned short));

                //list_add_item_to_tail(state->pending_requests, new_id(destination_id));
            }  else {
                // TODO: how to cancel sending?
                assert(false);
            }
        } else {
            // TODO: how to cancel sending?
            assert(false);
        }

        free(destination_id);
    }

    if(state->starvation->size > 0) {
        return SEND_NO_INC;
    }

    return NO_SEND;
}

// #define MSG_CONTROL_MESSAGE 1

static RoutingContextSendType BabelRoutingContextProcessMsg(ModuleState* m_state, const char* proto, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, unsigned short src_proto, byte* meta_data, unsigned int meta_length, bool new_seq, bool new_source, unsigned short my_seq, void* f_state) {

    BabelState* state = (BabelState*)m_state->vars;

    uuid_t parent_id;
    getParent(meta_data, meta_length, parent_id);


    RoutingNeighborsEntry* parent_entry = RN_getNeighbor(neighbors, parent_id);
    if( parent_entry && RNE_isBi(parent_entry)) { // parent is bi
        void* ptr = payload;

        byte type = 0;
        memcpy(&type, ptr, sizeof(byte));
        ptr += sizeof(byte);

        if(type == BABEL_UPDT) {

            list* to_update = list_init();
            list* to_remove = list_init();

            // process update
            processUpdate(state, SE_getID(source_entry), 0, 0, SE_getSEQ(source_entry), parent_entry, routing_table, neighbors, current_time, proto, to_update, to_remove);

            byte amount = 0;
            memcpy(&amount, ptr, sizeof(byte));
            ptr += sizeof(byte);

            for(int i = 0; i < amount; i++) {
                uuid_t destination_id;
                memcpy(destination_id, ptr, sizeof(uuid_t));
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

                //byte period_s = SE_getPeriod(source_entry);

                if(uuid_compare(destination_id, myID) != 0) {
                    // process update
                    processUpdate(state, destination_id, cost, hops, seq, parent_entry, routing_table, neighbors, current_time, proto, to_update, to_remove);
                }
            }

            RF_updateRoutingTable(routing_table, to_update, to_remove, current_time);

            if(state->starvation->size > 0 || state->trigger_update) {
                return SEND_NO_INC;
            }

        } else if(type == BABEL_SREQ) {

            uuid_t id;
            memcpy(id, ptr, sizeof(uuid_t));
            ptr += sizeof(uuid_t);

            unsigned short seq_no = 0;
            memcpy(&seq_no, ptr, sizeof(unsigned short));
            ptr += sizeof(unsigned short);

            char id_str1[UUID_STR_LEN];
            uuid_unparse(id, id_str1);

            char id_str2[UUID_STR_LEN];
            uuid_unparse(parent_id, id_str2);

            char str[200];
            sprintf(str, "to %s from %s SEQ=%hu", id_str1, id_str2, seq_no);

            my_logger_write(routing_logger, "BABEL", "RCV SEQREQ", str);

            // Add to pending_requests
            PendingReq* req = hash_table_find_value(state->pending_requests, id);
            if(req) {
                int cmp_seq = compare_seq(seq_no, req->seq_no, true);
                if(cmp_seq > 0 || compare_timespec(&req->expiration_time, current_time) <= 0 ) {
                    req->seq_no = seq_no;

                    struct timespec aux;
                    milli_to_timespec(&aux, 5*10*1000);
                    add_timespec(&req->expiration_time, &aux, current_time);
                }
            } else {
                PendingReq* req2 = malloc(sizeof(PendingReq));
                req2->seq_no = seq_no;

                struct timespec aux;
                milli_to_timespec(&aux, 5*10*1000);
                add_timespec(&req2->expiration_time, &aux, current_time);

                PendingReq* req3 = hash_table_insert(state->pending_requests, new_id(id), req2);
                assert(req3==NULL);
            }


            if(uuid_compare(id, myID) == 0) {
                if(seq_no > my_seq) {
                    return SEND_INC;
                } else {
                    return NO_SEND;
                }
            } else {
                // TODO

                return NO_SEND;
            }
        }
    }


    return NO_SEND;
}

RoutingContext* BabelRoutingContext() {

    BabelState* state = malloc(sizeof(BabelState));
    state->seq_req = false;
    state->destinations = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    state->starvation = list_init();
    state->trigger_update = false;
    state->pending_requests = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

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

static RoutingContextSendType ProcessDiscoveryEvent(YggEvent* ev, BabelState* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, const char* proto) {
    assert(ev);

    unsigned short ev_id = ev->notification_id;

    if(ev_id == UPDATE_NEIGHBOR || ev_id == LOST_NEIGHBOR) {
        recomputeAllRoutes(state, state->destinations, routing_table, neighbors, current_time, proto);
    }

    if(state->starvation->size > 0 || state->trigger_update) {
        return SEND_NO_INC;
    }

    return NO_SEND;
}


static bool isFeasible(FeasabilityDistance* feasability_distance, double cost, unsigned short seq_no) {

    if(cost == INFINITY) {
        return true;
    } else if(feasability_distance == NULL) {
        return true;
    } else {
        int seq_cmp = compare_seq(feasability_distance->seq_no, seq_no, true);

        return seq_cmp < 0 || (seq_cmp == 0 && cost < feasability_distance->cost);
    }
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



static void processUpdate(BabelState* state, unsigned char* destination_id, double cost, byte hops, unsigned short seq_no, RoutingNeighborsEntry* parent_entry, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto, list* to_update, list* to_remove) {

    char id_str1[UUID_STR_LEN];
    uuid_unparse(destination_id, id_str1);

    char id_str2[UUID_STR_LEN];
    uuid_unparse(RNE_getID(parent_entry), id_str2);

    //double route_cost = cost + RNE_getRxCost(parent_entry);
    //byte route_hops = hops + 1;

    char str[200];
    sprintf(str, "to %s from %s SEQ=%hu COST=%f HOPS=%d", id_str1, id_str2, seq_no, cost, hops);

    my_logger_write(routing_logger, "BABEL", "UPDATE", str);

    DestEntry* de = hash_table_find_value(state->destinations, destination_id);
    if(de == NULL) {
        de = malloc(sizeof(DestEntry));
        de->feasability_distance = NULL;
        de->neigh_vectors = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
        de->retracted = false;

        /*struct timespec aux;
        milli_to_timespec(&aux, 5*10*1000);
        add_timespec(&de->expiration_time, current_time, &aux);*/
        copy_timespec(&de->expiration_time, &zero_timespec);

        hash_table_insert(state->destinations, new_id(destination_id), de);
    }

    //
    PendingReq* req = hash_table_find_value(state->pending_requests, destination_id);
    if(req) {
        int seq_cmp = compare_seq(req->seq_no, seq_no, true);
        if(seq_cmp <= 0) {
            free(hash_table_remove(state->pending_requests, destination_id));
        }
    }

    // Route Acquisition
    bool is_feasible = isFeasible(de->feasability_distance, cost, seq_no);

    NeighEntry* neigh_vector = hash_table_find_value(de->neigh_vectors, RNE_getID(parent_entry));
    if(neigh_vector && compare_timespec(&neigh_vector->expiration_time, current_time) <= 0) {
        free(hash_table_remove(de->neigh_vectors, RNE_getID(parent_entry)));
        neigh_vector = NULL;
    }

    if(neigh_vector) {
        bool selected = false;
        RoutingTableEntry* rte = RT_findEntry(routing_table, destination_id);
        if(rte) {
            selected = uuid_compare(RNE_getID(parent_entry), RTE_getNextHopID(rte)) == 0;
        }

        if(!selected || is_feasible) {
            neigh_vector->seq_no = seq_no;
            neigh_vector->advertised_cost = cost;
            neigh_vector->advertised_hops = hops;

            // TODO: pass as args

            struct timespec aux;
            milli_to_timespec(&aux, 5*5*1000);
            add_timespec(&neigh_vector->expiration_time, current_time, &aux);
        }
    } else {
        if(is_feasible && cost != INFINITY) {
            neigh_vector = malloc(sizeof(NeighEntry));
            neigh_vector->seq_no = seq_no;
            neigh_vector->advertised_cost = cost;
            neigh_vector->advertised_hops = hops;

            struct timespec aux;
            milli_to_timespec(&aux, 5*5*1000);
            add_timespec(&neigh_vector->expiration_time, current_time, &aux);

            hash_table_insert(de->neigh_vectors, new_id(RNE_getID(parent_entry)), neigh_vector);
        }
    }

    // Update routing table, if necessary
    recomputeRoute(state, destination_id, de, routing_table, neighbors, current_time, proto, to_update, to_remove);
}


static void recomputeRoute(BabelState* state, unsigned char* destination_id, DestEntry* de, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto, list* to_update, list* to_remove) {

    //double* feasability_distance = SE_getAttr(destination, "feasability_distance");

    RoutingNeighborsEntry* best_neigh = NULL;
    double best_cost = INFINITY;
    byte best_hops = 255;
    unsigned int unfeasable_routes = 0;

    hash_table_item* current_neigh = NULL;
    void* iterator = NULL;
    while( (current_neigh = hash_table_iterator_next(de->neigh_vectors, &iterator)) ) {
        unsigned char* neigh_id = current_neigh->key;
        NeighEntry* neigh_vector = current_neigh->value;

        RoutingNeighborsEntry* rte = RN_getNeighbor(neighbors, neigh_id);
        if(rte) {
            double route_cost = neigh_vector->advertised_cost + RNE_getRxCost(rte);
            byte route_hops = neigh_vector->advertised_hops + 1;

            //bool new_seq_ = uuid_compare(neigh_id, RNE_getID(parent_entry)) == 0 ? new_seq : false;
            //bool new_seq_ = compare_seq(neigh_vector->seq_no, SE_getSEQ(destination), true) > 0;

            if(RNE_isBi(rte) && neigh_vector->advertised_cost != INFINITY) {

                if(isFeasible(de->feasability_distance, neigh_vector->advertised_cost, neigh_vector->seq_no)) {
                    bool select = route_cost < best_cost ? true : (route_cost == best_cost ? route_hops < best_hops : false);
                    if(select) {
                        best_neigh = rte;
                        best_cost = route_cost;
                        best_hops = route_hops;
                    }
                } else {
                    unfeasable_routes++;
                }
            }

        } else {
            //assert(false);

            // remove neigh from vector
            free(hash_table_remove(de->neigh_vectors, neigh_id));
        }
    }

    if(best_neigh) {
        de->retracted = false;

        // Update
        //RoutingNeighborsEntry* best_neigh_entry = RN_getNeighbor(neighbors, best_neigh->key);
        //assert(best_neigh_entry);

        RoutingTableEntry* rte = RT_findEntry(routing_table, destination_id);
        if(rte) {
            if(uuid_compare(RNE_getID(best_neigh), RTE_getNextHopID(rte)) != 0) {
                // different next hop
                state->trigger_update = true;
            }
        } else {
            // new route
            state->trigger_update = true;
        }

        //list* to_update = list_init();

        WLANAddr* addr = RNE_getAddr(best_neigh);
        RoutingTableEntry* new_entry = newRoutingTableEntry(destination_id, RNE_getID(best_neigh), addr, best_cost, best_hops, current_time, proto);

        list_add_item_to_tail(to_update, new_entry);

        //RF_updateRoutingTable(routing_table, to_update, NULL, current_time);
    } else {
        RoutingTableEntry* rte = RT_findEntry(routing_table, destination_id);

        if(rte) {
            // Remove if exist
            //list* to_remove = list_init();
            list_add_item_to_tail(to_remove, new_id(destination_id));
            //RF_updateRoutingTable(routing_table, NULL, to_remove, current_time);

            de->retracted = true;

            state->trigger_update = true;
        }

        if(unfeasable_routes > 0) {

            // check if in pending
            bool send = false;
            PendingReq* req = hash_table_find_value(state->pending_requests, destination_id);
            if(req) {
                int seq_cmp = compare_seq(req->seq_no, de->feasability_distance->seq_no, true);
                if(seq_cmp < 0|| compare_timespec(&req->expiration_time, current_time) <= 0) {
                    send = true;

                    req->seq_no = de->feasability_distance->seq_no;

                    struct timespec aux;
                    milli_to_timespec(&aux, 5*10*1000);
                    add_timespec(&req->expiration_time, &aux, current_time);
                }
            } else {
                send = true;

                PendingReq* req2 = malloc(sizeof(PendingReq));
                req2->seq_no = de->feasability_distance->seq_no;

                struct timespec aux;
                milli_to_timespec(&aux, 5*10*1000);
                add_timespec(&req2->expiration_time, &aux, current_time);

                PendingReq* req3 = hash_table_insert(state->pending_requests, new_id(destination_id), req2);
                assert(req3 == NULL);
            }

            if(send) {
                list_add_item_to_tail(state->starvation, new_id(destination_id));
            }

            char id_str[UUID_STR_LEN];
            uuid_unparse(destination_id, id_str);

            char str[200];
            sprintf(str, "to %s (%s)", id_str, (send?"sending":"not sending"));

            my_logger_write(routing_logger, "BABEL", "STARVATION", str);

        }
    }

}

static void recomputeAllRoutes(BabelState* state, hash_table* destinations, RoutingTable* routing_table, RoutingNeighbors* neighbors, struct timespec* current_time, const char* proto) {

    list* to_update = list_init();
    list* to_remove = list_init();

    // Verify if all routes are still valid and recompute if necessary
    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(destinations, &iterator)) ) {
        unsigned char* destination_id = hit->key;
        DestEntry* de = hit->value;

        recomputeRoute(state, destination_id, de, routing_table, neighbors, current_time, proto, to_update, to_remove);
    }

    RF_updateRoutingTable(routing_table, to_update, to_remove, current_time);
}
