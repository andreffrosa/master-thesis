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

#include <assert.h>

static unsigned int get_degree(unsigned char* id, RetransmissionContext* r_context, unsigned char* myID) {
    unsigned int n_neighbors = 0;
    if(!query_context(r_context, "degree", &n_neighbors, myID, 1, id))
        assert(false);

    return n_neighbors;
}

static bool priority_condition(unsigned char* w, unsigned char* v, RetransmissionContext* r_context, unsigned char* myID) {

    unsigned int deg_w = get_degree(w, r_context, myID);
    unsigned int deg_v = get_degree(v, r_context, myID);

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

static list* get_neighbors(unsigned char* id, RetransmissionContext* r_context, unsigned char* myID) {
    list* neighbors = NULL;
    if(!query_context(r_context, "neighbors", &neighbors, myID, 1, id))
        assert(false);

    return neighbors;
}

static bool _LENWBPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {
    list* neighbors = NULL;
    if(!query_context(r_context, "neighbors", &neighbors, myID, 0))
        assert(false);

    list* covered = NULL;
    if(!query_context(r_context, "coverage", &covered, myID, 2, p_msg, "first"))
        assert(false);

    unsigned char* v = malloc(sizeof(uuid_t));
    uuid_copy(v, myID);

    bool result = true;

    if(list_contained(neighbors, covered, &equalID, true)) {
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

            if(priority_condition(x, v, r_context, myID)) {
                if(list_find_item(neighbors, &equalID, x) != NULL) { // if(c belongs N(v))
                    list_add_item_to_tail(Q, x);
                } else {
                    list_add_item_to_tail(P, x);
                }
            }
        }

        while( !list_contained(neighbors, covered, &equalID, true) && (P->size > 0 || Q->size > 0) ) {

            if(Q->size > 0) {
                unsigned char* q = min_id(Q);
                list_add_item_to_tail(U, q);

                list* n_q = get_neighbors(q, r_context, myID);
                list_append(covered, n_q);

                for(list_item* it = n_q->head; it; it = it->next) {
                    unsigned char* x = malloc(sizeof(uuid_t));
                    uuid_copy(x, it->data);

                    if(priority_condition(x, v, r_context, myID) && list_find_item(U, &equalID, x) == NULL) {
                        if(list_find_item(neighbors, &equalID, x) != NULL) { // if(c belongs N(v))
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

                list* n_p = get_neighbors(p, r_context, myID);
                list_append(covered, n_p);

                for(list_item* it = n_p->head; it; it = it->next) {
                    unsigned char* x = malloc(sizeof(uuid_t));
                    uuid_copy(x, it->data);

                    if(priority_condition(x, v, r_context, myID)
                       && list_find_item(U, &equalID, x) == NULL
                       && list_find_item(neighbors, &equalID, x) != NULL) {
                                list_add_item_to_tail(Q, x);
                    }
                }

                free(list_remove_item(P, &equalID, p));
            }
        }

        result = !list_contained(neighbors, covered, &equalID, true);

        // Free
        list_delete(P);
        list_delete(Q);
        list_delete(U);
    }

    // Free
    list_delete(neighbors);
    list_delete(covered);

    free(v);

	return result;
}

RetransmissionPolicy* LENWBPolicy() {
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = NULL;
	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_LENWBPolicy;
    r_policy->destroy = NULL;

	return r_policy;
}
