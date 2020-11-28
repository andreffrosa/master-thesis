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

static bool _NeighborCountingPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {
	unsigned int c = *((unsigned int*)(policy_state->args));
	unsigned int n_copies = getCopies(p_msg)->size;
	unsigned int n = 0;
	if(!query_context(r_context, "in_degree", &n, myID, 0))
		assert(false);

	return n_copies < lMin(n, c);
}

static void _NeighborCountingPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* NeighborCountingPolicy(unsigned int c) {
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = malloc(sizeof(c));
	*((unsigned int*)(r_policy->policy_state.args)) = c;
	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_NeighborCountingPolicy;
    r_policy->destroy = &_NeighborCountingPolicyDestroy;

	return r_policy;
}
