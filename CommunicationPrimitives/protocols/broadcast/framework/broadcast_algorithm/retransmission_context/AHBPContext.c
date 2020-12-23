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
#include "utility/my_string.h"

/*static */list* compute_bgrs(graph* neighborhood, unsigned char* myID, list* covered);

static void AHBPContextAppendHeaders(ModuleState* context_state, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time) {
    list* covered = list_init();

    double_list* copies = getCopies(p_msg);
    assert(copies->size > 0);

    for(double_list_item* dit = copies->head; dit; dit = dit->next) {
        MessageCopy* copy = (MessageCopy*)dit->data;
        hash_table* headers = getHeaders(copy);

        list* route = (list*)hash_table_find_value(headers, "route");
        if(route) {
            list_append(covered, route);
        }
    }

    RetransmissionContext* neighbors_context = hash_table_find_value(contexts, "NeighborsContext");
    assert(neighbors_context);

    graph* neighborhood = NULL;
    if(!RC_query(neighbors_context, "neighborhood", &neighborhood, NULL, myID, contexts))
        assert(false);

    list* bgrs = compute_bgrs(neighborhood, myID, covered);

    if( bgrs->size > 0 ) {
        unsigned int size = bgrs->size * sizeof(uuid_t);
        byte buffer[size];
        byte* ptr = buffer;

        for(list_item* it = bgrs->head; it; it = it->next) {
            uuid_copy(ptr, it->data);
            ptr += sizeof(uuid_t);
        }

        appendHeader(serialized_headers, "delegated_neighbors", buffer, size);
    }

    graph_delete(neighborhood);
    list_delete(covered);
    list_delete(bgrs);
}

static void AHBPContextParseHeaders(ModuleState* context_state, hash_table* serialized_headers, hash_table* headers, unsigned char* myID) {
    list* bgrs = list_init();

    byte* buffer = (byte*)hash_table_find_value(serialized_headers, "delegated_neighbors");
    if(buffer) {
        byte* ptr = buffer;

        byte size = 0;
        memcpy(&size, ptr, sizeof(byte));
        ptr += sizeof(byte);

        int n = size / sizeof(uuid_t);
        for(int i = 0; i < n; i++) {
            unsigned char* id = malloc(sizeof(uuid_t));
            memcpy(id, ptr, sizeof(uuid_t));
            ptr += sizeof(uuid_t);

            list_add_item_to_tail(bgrs, id);
        }
    }

    //const char* key_ = "bgrs";
    const char* key_ = "delegated_neighbors";
    char* key = malloc(strlen(key_)+1);
    strcpy(key, key_);
    hash_table_insert(headers, key, bgrs);
}

RetransmissionContext* AHBPContext(RetransmissionContext* neighbors_context, RetransmissionContext* route_context) {

    RetransmissionContext* ctx = newRetransmissionContext(
        "AHBPContext",
        NULL,
        NULL,
        NULL,
        NULL,
        &AHBPContextAppendHeaders,
        &AHBPContextParseHeaders,
        NULL,
        NULL,
        NULL,
        new_list(2, new_str("NeighborsContext"), new_str("RouteContext"))
    );

    return ctx;
}

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
