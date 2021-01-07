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
    hash_table* topology_set;
    bool dirty;
} OLSRState;

typedef struct LinkEntry_ {
    uuid_t to;
    double tx_cost;
} LinkEntry;

typedef struct RouterSetEntry_ {
    unsigned short seq;
    struct timespec exp_time;
    list* links;
} RouterSetEntry;

static void RecomputeRoutingTable(hash_table* topology_set, RoutingNeighbors* neighbors, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time);

/*static void OLSRRoutingContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time) {

}*/

static bool OLSRRoutingContextTriggerEvent(ModuleState* m_state, unsigned short seq, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, unsigned char* myID, struct timespec* current_time, YggMessage* msg) {
    OLSRState* state = (OLSRState*)m_state->vars;

    if(event_type == RTE_NEIGHBORS_CHANGE) {
        YggEvent* ev = args;
        assert(ev);

        printf("XE\n");

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
        //RecomputeRoutingTable(state->topology_set, neighbors, myID, routing_table, current_time);

        return false;
    }

    else if(event_type == RTE_ANNOUNCE_TIMER) {

        printf("XO\n");

        if(state->dirty) {
            // TODO: inc seq
            state->dirty = false;
        }

        YggMessage_addPayload(msg, (char*)myID, sizeof(uuid_t));

        YggMessage_addPayload(msg, (char*)&seq, sizeof(unsigned short));

        byte period = 5; // TODO: como obter o period?
        YggMessage_addPayload(msg, (char*)&period, sizeof(byte));

        byte amount = state->mpr_selectors->size;
        YggMessage_addPayload(msg, (char*)&amount, sizeof(byte));

        for(list_item* it = state->mpr_selectors->head; it; it = it->next) {
            unsigned char* id = (unsigned char*)it->data;

            RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, id);
            assert(neigh);
            double tx_cost = RNE_getCost(neigh);

            YggMessage_addPayload(msg, (char*)id, sizeof(uuid_t));
            YggMessage_addPayload(msg, (char*)&tx_cost, sizeof(double));
        }

        printf("Sending TC msg seq = %hu amount=%d\n", seq, amount);

        return true;
    }

    return false;
}

static void OLSRRoutingContextRcvMsg(ModuleState* m_state, RoutingTable* routing_table, RoutingNeighbors* neighbors, unsigned char* myID, struct timespec* current_time, YggMessage* msg) {
    OLSRState* state = (OLSRState*)m_state->vars;

    void* ptr = NULL;

    uuid_t src;
    ptr = YggMessage_readPayload(msg, ptr, src, sizeof(uuid_t));

    unsigned short seq = 0;
    ptr = YggMessage_readPayload(msg, ptr, &seq, sizeof(unsigned short));

    byte period = 0;
    ptr = YggMessage_readPayload(msg, ptr, &period, sizeof(byte));

    byte amount = 0;
    ptr = YggMessage_readPayload(msg, ptr, &amount, sizeof(byte));

    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(src, id_str);
    printf("Received TC message from %s with seq %hu\n", id_str, seq);
    fflush(stdout);

    return; // discard; temp

    if( uuid_compare(src, myID) == 0 ) {
        return; // discard
    }

    RouterSetEntry* entry = hash_table_find_value(state->topology_set, src);

    if( entry && entry->seq > seq) {
        return; // discard msg
    }

    if(entry == NULL) {
        entry = malloc(sizeof(RouterSetEntry));

        entry->seq = seq;
        copy_timespec(&entry->exp_time, &zero_timespec);

        entry->links = list_init();

        hash_table_insert(state->topology_set, src, entry);
    }

    // TODO: fazer o exp

    if( seq > entry->seq) {
        entry->seq = seq;

        list_delete(entry->links);
        entry->links = list_init();

        for(int i = 0; i < amount; i++) {
            LinkEntry* link = malloc(sizeof(LinkEntry));
            ptr = YggMessage_readPayload(msg, ptr, link->to, sizeof(uuid_t));
            ptr = YggMessage_readPayload(msg, ptr, &link->tx_cost, sizeof(double));
            list_add_item_to_tail(entry->links, link);
        }

        // Recompute routing table
        //RecomputeRoutingTable(state->topology_set, neighbors, myID, routing_table, current_time);
    }

}

//typedef void (*rc_destroy)(ModuleState* m_state);


RoutingContext* OLSRRoutingContext() {

    OLSRState* state = malloc(sizeof(OLSRState));
    state->mprs = list_init();
    state->mpr_selectors = list_init();
    //state->router_set = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    state->topology_set = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    state->dirty = true;

    return newRoutingContext(
        NULL,
        state,
        NULL, //&OLSRRoutingContextInit,
        &OLSRRoutingContextTriggerEvent,
        &OLSRRoutingContextRcvMsg,
        NULL
    );
}

static void RecomputeRoutingTable(hash_table* topology_set, RoutingNeighbors* neighbors, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time) {
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
    }

    iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(topology_set, &iterator)) ) {
        RouterSetEntry* entry = (RouterSetEntry*)hit->value;
        unsigned char* entry_id = (unsigned char*)hit->key;

        for(list_item* it = entry->links->head; it; it= it->next) {
            LinkEntry* link = (LinkEntry*)it->data;

            if(graph_find_node(g, entry_id) == NULL) {
                graph_insert_node(g, new_id(entry_id), NULL);
            }

            if(graph_find_node(g, link->to) == NULL) {
                graph_insert_node(g, new_id(link->to), NULL);
            }

            assert(graph_find_edge(g, entry_id, link->to) == NULL);
            graph_insert_edge(g, entry_id, link->to, new_double(link->tx_cost));
        }
    }

    hash_table* routes = Dijkstra(g, myID);

    iterator = NULL;
    RoutingTableEntry* current_entry = NULL;
    while( (current_entry = RT_nextRoute(routing_table, &iterator)) ) {

        DijkstraTuple* dt = hash_table_remove(routes, RTE_getDestinationID(current_entry));
        if( dt ) {
            // Update Route
            unsigned char* next_hop = RTE_getNextHopID(current_entry);
            if(uuid_compare(next_hop, dt->next_hop_id) == 0) {
                // Just set cost
                RTE_setCost(current_entry, dt->cost);
            } else {
                // Set cost, next_hop and found_time and reset last used
                RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, dt->next_hop_id);
                assert(neigh);
                WLANAddr* addr = RNE_getAddr(neigh);
                RTE_setNexHop(current_entry, dt->next_hop_id, addr);

                RTE_setCost(current_entry, dt->cost);
                RTE_resetMessagesForwarded(current_entry);
                RTE_setFoundTime(current_entry, current_time);
                RTE_setLastUsedTime(current_entry, current_time);
            }

            free(dt);
        } else {
            RoutingTableEntry* aux = RT_removeEntry(routing_table, RTE_getDestinationID(current_entry));
            assert(aux == current_entry);
            free(aux);
        }
    }

    iterator = NULL;
    hit = NULL;
    while( (hit = hash_table_iterator_next(routes, &iterator)) ) {
        DijkstraTuple* dt = (DijkstraTuple*)hit->value;

        RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, dt->next_hop_id);
        assert(neigh);
        WLANAddr* addr = RNE_getAddr(neigh);

        RoutingTableEntry* route_entry = newRoutingTableEntry(dt->dest_id, dt->next_hop_id, addr, dt->cost, current_time);
        RT_addEntry(routing_table, route_entry);
    }

    hash_table_delete(routes);

    graph_delete(g);
}