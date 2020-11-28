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
 * (C) 2019
 *********************************************************/

#include "retransmission_policy_private.h"

#include "utility/my_math.h"

#include <assert.h>

static bool _EnhancedRapidPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {
	double beta = *((double*)(policy_state->args));
	unsigned int n = 0;
	if(!query_context(r_context, "n_neighbors", &n, myID, 0))
		assert(false);
	unsigned int n_copies = getCopies(p_msg)->size;
	unsigned int current_phase = getCurrentPhase(p_msg);

	double p  = dMin(1.0, (beta / n));
	double u = randomProb();

	if(current_phase == 1) {
		return (u <= p) && n_copies == 1;
	} else {
        bool retransmitted = getPhaseDecision(getPhaseStats(p_msg, getCurrentPhase(p_msg)-1));
		if(!retransmitted) {
			unsigned int copies_on_2nd_phase = 0;

			double_list_item* it = getCopies(p_msg)->head;
			while(it) {
                if(getCopyPhase(((message_copy*)it->data)) == 2) {
					copies_on_2nd_phase++;
				}
				it = it->next;
			}

			return copies_on_2nd_phase == 0;
		} else
			return false;
	}
}

static void _EnhancedRapidPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* EnhancedRapidPolicy(double beta) {
    RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = malloc(sizeof(double));
	*((double*)r_policy->policy_state.args) = beta;

	r_policy->policy_state.vars = NULL;
	r_policy->r_policy = &_EnhancedRapidPolicy;
    r_policy->destroy = &_EnhancedRapidPolicyDestroy;

	return r_policy;
}
