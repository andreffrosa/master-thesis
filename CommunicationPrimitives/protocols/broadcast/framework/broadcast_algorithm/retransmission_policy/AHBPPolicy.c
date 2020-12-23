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

static bool AHBPPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {

    bool mobility_extension = *((bool*)policy_state->args);
    RetransmissionPolicy* delegated_neighbors_policy = ((RetransmissionPolicy*)policy_state->vars);

    bool retransmit = RP_eval(delegated_neighbors_policy, p_msg, myID, contexts);

    if( mobility_extension && !retransmit ) {
        MessageCopy* first = ((MessageCopy*)getCopies(p_msg)->head->data);
        unsigned char* parent = getBcastHeader(first)->sender_id;

        RetransmissionContext* r_context = hash_table_find_value(contexts, "NeighborsContext");
        assert(r_context);

        graph* neighborhood = NULL;
        if(!RC_query(r_context, "neighborhood", &neighborhood, NULL, myID, contexts))
            assert(false);

        // parent is not my neighbor yet
        list* neighs = graph_get_adjacencies(neighborhood, myID, IN_ADJ);
        if(list_find_item(neighs, &equalID, parent) == NULL)
            retransmit = true;
        free(neighs);

        if(!retransmit) {
            // parent does not know me yet
            list* parent_neighs = graph_get_adjacencies(neighborhood, parent, IN_ADJ);
            if(list_find_item(parent_neighs, &equalID, myID) == NULL)
                retransmit = true;
            free(parent_neighs);
        }

        graph_delete(neighborhood);
    }

	return retransmit;
}

static void AHBPPolicyDestroy(ModuleState* policy_state) {
    RetransmissionPolicy* delegated_neighbors_policy = ((RetransmissionPolicy*)policy_state->vars);

    destroyRetransmissionPolicy(delegated_neighbors_policy);

    free(policy_state->args);
}

RetransmissionPolicy* AHBPPolicy(bool mobility_extension) { // ex = ahbp-ex : variant adapted to mobility

    bool* mobility_extension_arg = malloc(sizeof(int));
    *mobility_extension_arg = mobility_extension;

    RetransmissionPolicy* r_policy = newRetransmissionPolicy(
        mobility_extension_arg,
        DelegatedNeighborsPolicy(),
        &AHBPPolicyEval,
        &AHBPPolicyDestroy,
        new_list(1, new_str("AHBPContext"))
    );

    return r_policy;
}
