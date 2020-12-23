/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Author:
 * André Rosa (af.rosa@campus.fct.unl.pt)
 * Under the guidance of:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2019
 *********************************************************/

#include "retransmission_policy_private.h"

#include "data_structures/graph.h"
#include "utility/my_misc.h"
#include "utility/my_string.h"

#include <assert.h>

static list* get_bi_neighbors(unsigned char* id, hash_table* contexts, unsigned char* myID) {
    list* neighbors = NULL;

    hash_table* query_args = NULL;
    if(id) {
        query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
        char* key = malloc(3*sizeof(char));
        strcpy(key, "id");
        unsigned char* value = malloc(sizeof(uuid_t));
        uuid_copy(value, id);
        hash_table_insert(query_args, key, value);
    }

    RetransmissionContext* r_context = hash_table_find_value(contexts, "NeighborsContext");
    assert(r_context);

    if(!RC_query(r_context, "bi_neighbors", &neighbors, query_args, myID, contexts))
        assert(false);

    return neighbors;
}

static hash_table* get_two_hop_n_neighs(hash_table* contexts, unsigned char* myID) {
    hash_table* two_hop_n_neighs = NULL;

    RetransmissionContext* r_context = hash_table_find_value(contexts, "LENWBContext");
    assert(r_context);

    if(!RC_query(r_context, "LENWB_NEIGHS", &two_hop_n_neighs, NULL, myID, contexts))
        assert(false);

    return two_hop_n_neighs;
}

static list* get_covered(PendingMessage* p_msg, hash_table* contexts, unsigned char* myID) {
    list* covered = NULL;

    hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
    char* key = malloc(6*sizeof(char));
    strcpy(key, "p_msg");
    void** value = malloc(sizeof(void*));
    *value = p_msg;
    hash_table_insert(query_args, key, value);

    RetransmissionContext* r_context = hash_table_find_value(contexts, "NeighborsContext");
    assert(r_context);

    if(!RC_query(r_context, "coverage", &covered, query_args, myID, contexts))
        assert(false);

    return covered;
}

static unsigned int get_degree(unsigned char* id, hash_table* contexts, unsigned char* myID, list* bi_neighbors, hash_table* two_hop_n_neighs) {
    if( uuid_compare(id, myID) == 0 ) {
        return bi_neighbors->size;
    } else {
        if( list_find_item(bi_neighbors, &equalID, id) ) {
            unsigned int n_neighbors = 0;

            hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
            char* key = malloc(3*sizeof(char));
            strcpy(key, "id");
            unsigned char* value = malloc(sizeof(uuid_t));
            uuid_copy(value, id);
            hash_table_insert(query_args, key, value);

            RetransmissionContext* r_context = hash_table_find_value(contexts, "NeighborsContext");
            assert(r_context);

            if(!RC_query(r_context, "degree", &n_neighbors, query_args, myID, contexts))
                assert(false);

            return n_neighbors;
        } else {
            byte* n_neighbors = hash_table_find_value(two_hop_n_neighs, id);
            if(n_neighbors) {
                return *n_neighbors;
            } else {
                return 0; // TODO: what to return?
            }
        }
    }
}

static bool priority_condition(unsigned char* w, unsigned char* v, hash_table* contexts, unsigned char* myID, list* bi_neighbors, hash_table* two_hop_n_neighs) {

    unsigned int deg_w = get_degree(w, contexts, myID, bi_neighbors, two_hop_n_neighs);
    unsigned int deg_v = get_degree(v, contexts, myID, bi_neighbors, two_hop_n_neighs);

    return deg_w > deg_v || (deg_w == deg_v && uuid_compare(w, v) < 0);
}

static unsigned char* min_id(list* l) {
    unsigned char* m = NULL;

    for(list_item* it = l->head; it; it = it->next) {
        if(m == NULL || uuid_compare(m, it->data) < 0 )
            m = it->data;
    }

    if(m){
        unsigned char* aux = malloc(sizeof(uuid_t));
        uuid_copy(aux, m);
        return aux;
    }

    return NULL;
}

static bool LENWBPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {

    list* bi_neighbors = get_bi_neighbors(NULL, contexts, myID);
    hash_table* two_hop_n_neighs = get_two_hop_n_neighs(contexts, myID);
    list* covered = get_covered(p_msg, contexts, myID);

    unsigned char* v = malloc(sizeof(uuid_t));
    uuid_copy(v, myID);

    bool result = true;

    if(list_contained(bi_neighbors, covered, &equalID, true)) {
        result = false;
    } else {
        list* P = list_init();
        list* Q = list_init();
        list* U = list_init();

        unsigned char* u = malloc(sizeof(uuid_t));
        uuid_copy(u, getCopies(p_msg)->head->data);
        list_add_item_to_tail(U, u);

        for(list_item* it = covered->head; it; it = it->next) {
            unsigned char* x = malloc(sizeof(uuid_t));
            uuid_copy(x, it->data);

            if(priority_condition(x, v, contexts, myID, bi_neighbors, two_hop_n_neighs)) {
                if(list_find_item(bi_neighbors, &equalID, x) != NULL) { // if(c belongs N(v))
                    list_add_item_to_tail(Q, x);
                } else {
                    list_add_item_to_tail(P, x);
                }
            }
        }

        while( !list_contained(bi_neighbors, covered, &equalID, true) && (P->size > 0 || Q->size > 0) ) {

            if(Q->size > 0) {
                unsigned char* q = min_id(Q);
                list_add_item_to_tail(U, q);

                list* n_q = get_bi_neighbors(q, contexts, myID);
                list_append(covered, n_q);

                for(list_item* it = n_q->head; it; it = it->next) {
                    unsigned char* x = malloc(sizeof(uuid_t));
                    uuid_copy(x, it->data);

                    if(priority_condition(x, v, contexts, myID, bi_neighbors, two_hop_n_neighs) && list_find_item(U, &equalID, x) == NULL) {
                        if(list_find_item(bi_neighbors, &equalID, x) != NULL) { // if(c belongs N(v))
                            list_add_item_to_tail(Q, x);
                        } else {
                            list_add_item_to_tail(P, x);
                        }
                    }
                }

                free(list_remove_item(Q, &equalID, q));
            } else {
                unsigned char* p = min_id(P);
                list_add_item_to_tail(U, p);

                list* n_p = get_bi_neighbors(p, contexts, myID);
                list_append(covered, n_p);

                for(list_item* it = n_p->head; it; it = it->next) {
                    unsigned char* x = malloc(sizeof(uuid_t));
                    uuid_copy(x, it->data);

                    if(priority_condition(x, v, contexts, myID, bi_neighbors, two_hop_n_neighs)
                       && list_find_item(U, &equalID, x) == NULL
                       && list_find_item(bi_neighbors, &equalID, x) != NULL) {
                                list_add_item_to_tail(Q, x);
                    }
                }

                free(list_remove_item(P, &equalID, p));
            }
        }

        result = !list_contained(bi_neighbors, covered, &equalID, true);

        // Free
        list_delete(P);
        list_delete(Q);
        list_delete(U);
    }

    // Free
    list_delete(bi_neighbors);
    list_delete(covered);

    free(v);

	return result;
}

RetransmissionPolicy* LENWBPolicy() {
	RetransmissionPolicy* r_policy = newRetransmissionPolicy(
        NULL,
        NULL,
        &LENWBPolicyEval,
        NULL,
        new_list(1, new_str("LENWBContext"))
    );

    return r_policy;
}
