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

#include "utility/olsr_utils.h"

#include <assert.h>

typedef struct OLSRState_ {
    list* mprs;
    list* mpr_selectors;
    //hash_table* router_set;
    //hash_table* topology_set;
    bool dirty;
} OLSRState;

typedef struct LinkEntry_ {
    uuid_t to;
    double tx_cost;
} LinkEntry;

//typedef struct RouterSetEntry_ {
//unsigned short seq;
//struct timespec exp_time;
//list* links;
//} RouterSetEntry;

static void RecomputeRoutingTable(SourceTable* source_table, RoutingNeighbors* neighbors, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time);

static bool ProcessDiscoveryEvent(YggEvent* ev, OLSRState* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time);

/*static void OLSRRoutingContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time) {

}*/

static RoutingContextSendType OLSRRoutingContextTriggerEvent(ModuleState* m_state, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {
    OLSRState* state = (OLSRState*)m_state->vars;

    if(event_type == RTE_NEIGHBORS_CHANGE) {
        YggEvent* ev = args;
        return ProcessDiscoveryEvent(ev, state, routing_table, neighbors, source_table, myID, current_time);
    }

    else if(event_type == RTE_ANNOUNCE_TIMER) {

        byte amount = state->mpr_selectors->size;
        if( state->dirty ) {
            state->dirty = false;
            return SEND_INC;
        } else if( amount > 0 ) {
            return SEND_NO_INC;
        } else {
            return NO_SEND;
        }
    }

    return NO_SEND;
}

static void OLSRRoutingContextCreateMsg(ModuleState* m_state, unsigned short seq, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg) {
    OLSRState* state = (OLSRState*)m_state->vars;

    YggMessage_addPayload(msg, (char*)myID, sizeof(uuid_t));

    YggMessage_addPayload(msg, (char*)&seq, sizeof(unsigned short));

    byte period = 5; // TODO: como obter o period?
    YggMessage_addPayload(msg, (char*)&period, sizeof(byte));

    byte amount = state->mpr_selectors->size;
    YggMessage_addPayload(msg, (char*)&amount, sizeof(byte));

    printf("MPR SELECTORS IN MY TC:\n");
    fflush(stdout);

    for(list_item* it = state->mpr_selectors->head; it; it = it->next) {
        unsigned char* id = (unsigned char*)it->data;

        char str[UUID_STR_LEN];
        uuid_unparse(id, str);
        printf("%s\n", str);
        fflush(stdout);

        RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, id);
        assert(neigh);
        double tx_cost = RNE_getCost(neigh);

        YggMessage_addPayload(msg, (char*)id, sizeof(uuid_t));
        YggMessage_addPayload(msg, (char*)&tx_cost, sizeof(double));
    }

    printf("Sending TC msg seq = %hu amount=%d\n", seq, amount);
}

static void OLSRRoutingContextProcessMsg(ModuleState* m_state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg) {
    //OLSRState* state = (OLSRState*)m_state->vars;

    void* ptr = NULL;

    uuid_t source_id;
    ptr = YggMessage_readPayload(msg, ptr, source_id, sizeof(uuid_t));

    unsigned short seq = 0;
    ptr = YggMessage_readPayload(msg, ptr, &seq, sizeof(unsigned short));

    byte period = 0;
    ptr = YggMessage_readPayload(msg, ptr, &period, sizeof(byte));

    byte amount = 0;
    ptr = YggMessage_readPayload(msg, ptr, &amount, sizeof(byte));

    list* links = list_init();
    for(int i = 0; i < amount; i++) {
        LinkEntry* link = malloc(sizeof(LinkEntry));
        ptr = YggMessage_readPayload(msg, ptr, link->to, sizeof(uuid_t));
        ptr = YggMessage_readPayload(msg, ptr, &link->tx_cost, sizeof(double));
        list_add_item_to_tail(links, link);
    }

    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(source_id, id_str);
    printf("Received TC message from %s with seq %hu\n", id_str, seq);
    fflush(stdout);

    if( uuid_compare(source_id, myID) == 0 ) {
        return; // discard
    }

    unsigned int misses = 3; // TODO:

    struct timespec exp_time = {0};
    milli_to_timespec(&exp_time, period*misses*1000);
    add_timespec(&exp_time, &exp_time, current_time);

    SourceEntry* entry = SS_getEntry(source_table, source_id);
    if(entry == NULL) {
        entry = newSourceEntry(source_id, seq, &exp_time, links);

        SS_addEntry(source_table, entry);

        // Recompute routing table
        RecomputeRoutingTable(source_table, neighbors, myID, routing_table, current_time);
    } else {
        if( seq > SE_getSEQ(entry) ) {
            SE_setSEQ(entry, seq);

            list_delete(SE_getAttrs(entry));
            SE_setAttrs(entry, links);

            // Recompute routing table
            RecomputeRoutingTable(source_table, neighbors, myID, routing_table, current_time);
        } else {
            list_delete(links);
        }
    }

}

//typedef void (*rc_destroy)(ModuleState* m_state);


RoutingContext* OLSRRoutingContext() {

    OLSRState* state = malloc(sizeof(OLSRState));
    state->mprs = list_init();
    state->mpr_selectors = list_init();
    //state->router_set = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    //state->topology_set = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    state->dirty = true;

    return newRoutingContext(
        NULL,
        state,
        NULL, //&OLSRRoutingContextInit,
        &OLSRRoutingContextTriggerEvent,
        &OLSRRoutingContextCreateMsg,
        &OLSRRoutingContextProcessMsg,
        NULL
    );
}

static void RecomputeRoutingTable(SourceTable* source_table, RoutingNeighbors* neighbors, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time) {
    graph* g = graph_init_complete((key_comparator)&uuid_compare, NULL, NULL, sizeof(uuid_t), 0, sizeof(double));

    graph_insert_node(g, new_id(myID), NULL);

    void* iterator = NULL;
    RoutingNeighborsEntry* neigh = NULL;
    while( (neigh = RN_nextNeigh(neighbors, &iterator)) ) {
        unsigned char* neigh_id = RNE_getID(neigh);
        assert(graph_find_node(g, neigh_id) == NULL);
        graph_insert_node(g, new_id(neigh_id), NULL);

        graph_insert_edge(g, myID, neigh_id, new_double(RNE_getCost(neigh)));
        graph_insert_edge(g, neigh_id, myID, new_double(RNE_getCost(neigh)));

        /*
        char str[UUID_STR_LEN];
        uuid_unparse(neigh_id, str);
        printf("neigh_id: %s\n", str);
        */
    }

    //printf("tc:\n");

    iterator = NULL;
    SourceEntry* entry = NULL;
    while( (entry = SS_nexEntry(source_table, &iterator)) ) {

        list* links = SE_getAttrs(entry);

        for(list_item* it = links->head; it; it= it->next) {
            LinkEntry* link = (LinkEntry*)it->data;

            if(graph_find_node(g, SE_getID(entry)) == NULL) {
                graph_insert_node(g, new_id(SE_getID(entry)), NULL);
            }

            if(graph_find_node(g, link->to) == NULL) {
                graph_insert_node(g, new_id(link->to), NULL);
            }

            //bool inserted = false;

            if(graph_find_edge(g, SE_getID(entry), link->to) == NULL) {
                graph_insert_edge(g, SE_getID(entry), link->to, new_double(link->tx_cost));
                //inserted = true;
            }

            /*
            char from_str[UUID_STR_LEN];
            uuid_unparse(entry_id, from_str);

            char to_str[UUID_STR_LEN];
            uuid_unparse(link->to, to_str);

            printf("%s -> %s : %f %s\n", from_str, to_str, link->tx_cost, (inserted?"T":"F"));
            */


        }
    }

    //printf("graph:\n");

    // Temp
    for(list_item* it = g->edges->head; it; it = it->next) {
        graph_edge* edge = (graph_edge*)it->data;

        char from_str[UUID_STR_LEN];
        uuid_unparse(edge->start_node->key, from_str);

        char to_str[UUID_STR_LEN];
        uuid_unparse(edge->end_node->key, to_str);

        double* cost = (double*)edge->label;
        assert(cost);

        //printf("%s -> %s : %f\n", from_str, to_str, *cost);
    }

    hash_table* routes = Dijkstra(g, myID);

    //printf("routes %d\n", routes->n_items);

    list* to_update = list_init();
    list* to_remove = list_init();

    iterator = NULL;
    RoutingTableEntry* current_entry = NULL;
    while( (current_entry = RT_nextRoute(routing_table, &iterator)) ) {

        DijkstraTuple* dt = hash_table_remove(routes, RTE_getDestinationID(current_entry));
        if( dt ) {
            RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, dt->next_hop_id);
            assert(neigh);
            WLANAddr* addr = RNE_getAddr(neigh);

            RoutingTableEntry* new_entry = newRoutingTableEntry(dt->destination_id, dt->next_hop_id, addr, dt->cost, dt->hops, current_time);

            list_add_item_to_tail(to_update, new_entry);

            free(dt);
        } else {
            RoutingTableEntry* old_entry = RT_findEntry(routing_table, RTE_getDestinationID(current_entry));
            assert(old_entry == current_entry);

            list_add_item_to_tail(to_remove, new_id(RTE_getDestinationID(old_entry)));


            //free(old_entry);
        }
    }

    iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(routes, &iterator)) ) {
        DijkstraTuple* dt = (DijkstraTuple*)hit->value;

        RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, dt->next_hop_id);
        assert(neigh);
        WLANAddr* addr = RNE_getAddr(neigh);

        RoutingTableEntry* new_entry = newRoutingTableEntry(dt->destination_id, dt->next_hop_id, addr, dt->cost, dt->hops, current_time);

        list_add_item_to_tail(to_update, new_entry);
    }

    hash_table_delete(routes);
    graph_delete(g);

    RF_updateRoutingTable(routing_table, to_update, to_remove, current_time);
}

static bool ProcessDiscoveryEvent(YggEvent* ev, OLSRState* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {
    assert(ev);

    //printf("XE\n");

    if(ev->notification_id == GENERIC_DISCOVERY_EVENT) {

        unsigned int str_len = 0;
        void* ptr = NULL;
        ptr = YggEvent_readPayload(ev, ptr, &str_len, sizeof(unsigned int));

        char type[str_len+1];
        ptr = YggEvent_readPayload(ev, ptr, type, str_len*sizeof(char));
        type[str_len] = '\0';

        if( strcmp(type, "MPRS") == 0 || strcmp(type, "MPR SELECTORS") == 0 ) {
            unsigned int amount = 0;
            ptr = YggEvent_readPayload(ev, ptr, &amount, sizeof(unsigned int));

            // Skip Flooding
            ptr += amount*sizeof(uuid_t);

            amount = 0;
            ptr = YggEvent_readPayload(ev, ptr, &amount, sizeof(unsigned int));

            list* l = list_init();

            for(int i = 0; i < amount; i++) {
                //uuid_t id;
                unsigned char* id = malloc(sizeof(uuid_t));
                ptr = YggEvent_readPayload(ev, ptr, id, sizeof(uuid_t));

                list_add_item_to_tail(l, id);

                /*
                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(id, id_str);
                printf("%s\n", id_str);
                */
            }

            if( strcmp(type, "MPRS") == 0  ) {
                list_delete(state->mprs);
                state->mprs = l;
            } else if( strcmp(type, "MPR SELECTORS") == 0 ) {
                list_delete(state->mpr_selectors);
                state->mpr_selectors = l;

                state->dirty = true;
            }
        }
    }

    // Recompute routing table
    RecomputeRoutingTable(source_table, neighbors, myID, routing_table, current_time);

    return state->dirty;
}