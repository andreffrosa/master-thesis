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
 * (C) 2020
 *********************************************************/

#include "retransmission_policy_private.h"

#include "utility/my_misc.h"

#include "data_structures/graph.h"

#include <assert.h>

static bool _AHBPPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {

    int ex = *((int*)policy_state->args);

    message_copy* first = ((message_copy*)getCopies(p_msg)->head->data);

    RetransmissionPolicy* delegated_neighbors_policy = ((RetransmissionPolicy*)policy_state->vars);

    bool retransmit = delegated_neighbors_policy->r_policy(&delegated_neighbors_policy->policy_state, p_msg, r_context, myID);

    if( ex > 0 && !retransmit ) {
        unsigned char* parent = getBcastHeader(first)->sender_id;

        graph* real_neighborhood;
        if(!query_context(r_context, "neighborhood", &real_neighborhood, myID, 0))
            assert(false);

        if(ex > 1) {
            list* neighs = graph_get_adjacencies(real_neighborhood, myID, IN_ADJ);
            if(list_find_item(neighs, &equalID, parent) == NULL) // parent is not my neighbor yet
                retransmit = true;
            free(neighs);
        }

        if( ex == 2 && !retransmit ) {
            list* parent_neighs = graph_get_adjacencies(real_neighborhood, parent, IN_ADJ);
            if(list_find_item(parent_neighs, &equalID, myID) == NULL) // parent does not know me yet
                retransmit = true;
            free(parent_neighs);
        }

        graph_delete(real_neighborhood);
    }

	return retransmit;
}

static void _AHBPPolicyDestroy(ModuleState* policy_state, list* visited) {
    RetransmissionPolicy* delegated_neighbors_policy = ((RetransmissionPolicy*)policy_state->vars);

    if(list_find_item(visited, &equalAddr, delegated_neighbors_policy) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = delegated_neighbors_policy;
        list_add_item_to_tail(visited, this);

        destroyRetransmissionPolicy(delegated_neighbors_policy, visited);
    }

    free(policy_state->args);
}

RetransmissionPolicy* AHBPPolicy(int ex) { // ex = ahbp-ex : variant adapted to mobility
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

    int* ex_ptr = malloc(sizeof(int));
    *ex_ptr = ex;
	r_policy->policy_state.args = ex_ptr;
	r_policy->policy_state.vars = DelegatedNeighborsPolicy();

	r_policy->r_policy = &_AHBPPolicy;
    r_policy->destroy = &_AHBPPolicyDestroy;

	return r_policy;
}
