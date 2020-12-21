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

#include "retransmission_context_private.h"

#include <assert.h>

#include "data_structures/graph.h"

#include "utility/olsr_utils.h"

typedef struct _AHBPContextArgs {
	RetransmissionContext* neighbors_context;
    RetransmissionContext* route_context;
} AHBPContextArgs;

static bool AHBPContextQueryHeader(ModuleState* context_state, void* header, unsigned int header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited);

/*static */list* compute_bgrs(graph* neighborhood, unsigned char* myID, list* covered) {

    for(list_item* it = covered->head; it; it = it->next) {
        if( uuid_compare(myID, it->data) != 0 ) {

            list* neighs = graph_get_adjacencies(neighborhood, it->data, SYM_ADJ); // OUD_ADJ?

            if( neighs ) {
                for(list_item* it2 = neighs->head; it2; it2 = it2->next) {
                    if( uuid_compare(myID, it2->data) != 0 ) {
                        void* x = graph_remove_node(neighborhood, it2->data);
                        if(x) free(x);
                    }
                }
            }

            void* x = graph_remove_node(neighborhood, it->data);
            if(x) free(x);

            list_delete(neighs);
        }
    }

    hash_table* n1 = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    list* n2 = list_init();

    list* one_hop_neighs = graph_get_adjacencies(neighborhood, myID, SYM_ADJ);
    for(list_item* it = one_hop_neighs->head; it; it = it->next) {
        unsigned char* neigh_id = (unsigned char*)it->data;

        list* ns = list_init();

        list* two_hop_neighs = graph_get_adjacencies(neighborhood, neigh_id, SYM_ADJ);
        for(list_item* it2 = two_hop_neighs->head; it2; it2 = it2->next) {
            unsigned char* two_hop_neigh_id = (unsigned char*)it2->data;

            //bool is_bi = ;
            bool is_not_me = uuid_compare(two_hop_neigh_id, myID) != 0;
            bool is_not_in_n2_yet = list_find_item(n2, &equalN2Tuple, two_hop_neigh_id) == NULL;

            if( /*is_bi &&*/ is_not_me) {
                if( is_not_in_n2_yet ) {
                    unsigned char* id = malloc(sizeof(uuid_t));
                    uuid_copy(id, two_hop_neigh_id);
                    list_add_item_to_tail(n2, id);
                }
                double* tx_lq = graph_find_label(neighborhood, neigh_id, two_hop_neigh_id);
                assert(tx_lq);
                N2_Tuple* n2_tuple = newN2Tuple(two_hop_neigh_id, *tx_lq); // lq from 1 hop to 2 hops
                list_add_item_to_tail(ns, n2_tuple);
            }
        }
        list_delete(two_hop_neighs);

        double* tx_lq = graph_find_label(neighborhood, myID, neigh_id);
        assert(tx_lq);
        N1_Tuple* n1_tuple = newN1Tuple(neigh_id, *tx_lq, DEFAULT_WILLINGNESS, ns, false);
        void* old = hash_table_insert(n1, n1_tuple->id, n1_tuple);
        assert(old == NULL);
    }
    list_delete(one_hop_neighs);

    list* bgrs = compute_multipoint_relays(n1, n2, NULL);

    hash_table_delete_custom(n1, &delete_n1_item, NULL);
    list_delete(n2);

    // Debug
    /*
printf("Computed Delegated Neighbors (BGRs %d):\n", bgrs->size);
    for(list_item* it = bgrs->head; it; it = it->next) {
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(((unsigned char*)it->data), id_str);
        printf("%s\n", id_str);
    }
    fflush(stdout);
*/

    return bgrs;
}

static void AHBPContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);

    RC_init(args->neighbors_context, protocol_definition, myID, visited);

    RC_init(args->route_context, protocol_definition, myID, visited);
}

static unsigned int append_bgrs(ModuleState* context_state, PendingMessage* p_msg, void** context_header, unsigned char* myID) {
    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);

    list* covered = list_init();

    double_list* copies = getCopies(p_msg);
    assert(copies->size > 0);

    for(double_list_item* dit = copies->head; dit; dit = dit->next) {
        MessageCopy* copy = (MessageCopy*)dit->data;

        list* visited2 = list_init();
        list* route = NULL;
        if(!AHBPContextQueryHeader(context_state, getContextHeader(copy), getBcastHeader(copy)->context_length, "route", &route, NULL, myID, visited2))
            assert(false);
        list_delete(visited2);

        if(route)
            list_append(covered, route);
    }

    list* visited2 = list_init();
    graph* neighborhood = NULL;
    if(!RC_query(args->neighbors_context, "neighborhood", &neighborhood, NULL, myID, visited2))
        assert(false);
    list_delete(visited2);

    list* bgrs = compute_bgrs(neighborhood, myID, covered);

    int bgrs_header_size = 0;
    unsigned char* bgrs_header = NULL;
    if(bgrs->size > 0) {
        bgrs_header_size = bgrs->size * sizeof(uuid_t);
        bgrs_header = malloc(bgrs_header_size);
        unsigned char* ptr = bgrs_header;

        for(list_item* it = bgrs->head; it; it = it->next) {
            uuid_copy(ptr, it->data);
            ptr += sizeof(uuid_t);
        }
    }

    graph_delete(neighborhood);
    list_delete(covered);
    list_delete(bgrs);

    *context_header = bgrs_header;
    return bgrs_header_size;
}

static unsigned int AHBPContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, unsigned char* myID, list* visited) {
    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);

    // Insert route
    void* route_header = NULL;
    unsigned int route_header_size = RC_createHeader(args->route_context, p_msg, &route_header, myID, visited);

    // Insert BGRs
    void* bgrs_header = NULL;
    unsigned int bgrs_header_size = append_bgrs(context_state, p_msg, &bgrs_header, myID);

    assert(route_header_size <= 255 && bgrs_header_size <= 255);
    assert((route_header_size == 0 || route_header !=NULL) && (route_header == NULL || route_header_size > 0));
    assert((bgrs_header_size == 0 || bgrs_header !=NULL) && (bgrs_header == NULL || bgrs_header_size > 0));

    unsigned short total_size = 2*sizeof(byte) + route_header_size + bgrs_header_size;

    byte aux = '\0';
    unsigned char* buffer = malloc(total_size);
    unsigned char* ptr = buffer;

    aux = route_header_size;
    memcpy(ptr, &aux, sizeof(byte));
    ptr += sizeof(byte);

    aux = bgrs_header_size;
    memcpy(ptr, &aux, sizeof(byte));
    ptr += sizeof(byte);

    if( route_header_size > 0 ) {
        memcpy(ptr, route_header, route_header_size);
        ptr += route_header_size;
    }

    if( bgrs_header_size > 0 ) {
        memcpy(ptr, bgrs_header, bgrs_header_size);
        ptr += bgrs_header_size;
    }

    free(route_header);
    free(bgrs_header);

    *context_header = buffer;
	return total_size;
}

static void AHBPContextEvent(ModuleState* context_state, queue_t_elem* elem, unsigned char* myID, list* visited) {
    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);

    RC_processEvent(args->neighbors_context, elem, myID, visited);

    RC_processEvent(args->route_context, elem, myID, visited);
}

static bool AHBPContextQuery(ModuleState* context_state, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {
    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);

    bool found = RC_query(args->route_context, query, result, query_args, myID, visited);

    if(!found) {
        return RC_query(args->neighbors_context, query, result, query_args, myID, visited);
    } else {
        return true;
    }

}

static bool AHBPContextQueryHeader(ModuleState* context_state, void* header, unsigned int header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {

    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);

    byte sizes[2];
    unsigned char* route_header = NULL;
    unsigned char* bgrs_header = NULL;

    if(header != NULL) {
        memcpy(sizes, header, sizeof(sizes));

        route_header = header + 2*sizeof(byte);
        bgrs_header = route_header + sizes[0];
    } else {
        memset(sizes, 0, sizeof(sizes));

        route_header = NULL;
        bgrs_header = NULL;
    }

    if( strcmp(query, "bgrs") == 0 || strcmp(query, "delegated_neighbors") == 0 ) {
        list* bgrs = list_init();

        int size = sizes[1] / sizeof(uuid_t);
        for(int i = 0; i < size; i++) {
            unsigned char* id = bgrs_header + i*sizeof(uuid_t);
            unsigned char* id_copy = malloc(sizeof(uuid_t));
            uuid_copy(id_copy, id);
            list_add_item_to_tail(bgrs, id_copy);
        }

		*((list**)result) = bgrs;
		return true;
	} if( strcmp(query, "delegated") == 0 ) {
        list* bgrs = list_init();

        int size = sizes[1] / sizeof(uuid_t);

        for(int i = 0; i < size; i++) {
            unsigned char* id = bgrs_header + i*sizeof(uuid_t);
            unsigned char* id_copy = malloc(sizeof(uuid_t));
            uuid_copy(id_copy, id);
            list_add_item_to_tail(bgrs, id_copy);
        }

        *((bool*)result) = (list_find_item(bgrs, &equalID, myID) != NULL);
        list_delete(bgrs);
		return true;
	} else {
        return RC_queryHeader(args->route_context, route_header, sizes[0], query, result, query_args, myID, visited);
    }

}

static void AHBPContextDestroy(ModuleState* context_state, list* visited) {
    AHBPContextArgs* args = context_state->args;

    destroyRetransmissionContext(args->route_context, visited);
    destroyRetransmissionContext(args->neighbors_context, visited);

    free(args);
}

RetransmissionContext* AHBPContext(RetransmissionContext* neighbors_context, RetransmissionContext* route_context) {

    AHBPContextArgs* args = malloc(sizeof(AHBPContextArgs));
    args->neighbors_context = neighbors_context;
    args->route_context = route_context;

    return newRetransmissionContext(
        args,
        NULL,
        &AHBPContextInit,
        &AHBPContextEvent,
        &AHBPContextHeader,
        &AHBPContextQuery,
        &AHBPContextQueryHeader,
        NULL,
        &AHBPContextDestroy
    );
}
