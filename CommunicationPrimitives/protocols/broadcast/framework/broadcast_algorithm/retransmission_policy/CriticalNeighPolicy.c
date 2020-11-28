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

#include "retransmission_policy_private.h"

#include <assert.h>

static bool _CriticalNeighPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {
	double min_critical_coverage =  *((double*)policy_state->args);
	unsigned int current_phase = getCurrentPhase(p_msg);

    // Check if the current node is critical to all the parents. In case it is not cirtical to some parent then all the neighbors already were convered.

    bool isCritical = true;

    double_list* copies = getCopies(p_msg);
    for(double_list_item* it = copies->head; it; it = it->next) {
        unsigned char* parent_id = getBcastHeader((message_copy*)it->data)->sender_id;

        LabelNeighs_NodeLabel in_label;
        if(!query_context(r_context, "node_in_label", &in_label, myID, 1, parent_id))
            assert(false);

        isCritical &= (in_label == CRITICAL_NODE || in_label == UNKNOWN_NODE);
    }

	if ( isCritical ) { // Only the critical  processes retransmit
		if(current_phase == 1) {
			return true;
		} else {
            // May lead to incorrect assumptions on the necessity of other nodes retransmiting, because some other node may turn them covered or redundant, and thus it increases the cost of the second phase
			double missed_critical_neighs;
            if(!query_context(r_context, "missed_critical_neighs", &missed_critical_neighs, myID, 1, p_msg))
                assert(false);

			if(missed_critical_neighs > min_critical_coverage)
				return true;
		}
	}
	return false;
}

static void _CriticalNeighPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* CriticalNeighPolicy(double min_critical_coverage) {
	assert( 0.0 <= min_critical_coverage && min_critical_coverage  <= 1.0 );

	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = malloc(sizeof(double));
	*((double*)r_policy->policy_state.args) = min_critical_coverage;
	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_CriticalNeighPolicy;
    r_policy->destroy = &_CriticalNeighPolicyDestroy;

	return r_policy;
}
