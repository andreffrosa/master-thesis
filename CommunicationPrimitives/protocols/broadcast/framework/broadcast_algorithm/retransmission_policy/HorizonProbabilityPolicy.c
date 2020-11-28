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

typedef struct _HorizonProbabilityPolicyArgs {
	double p;
	unsigned int k;
} HorizonProbabilityPolicyArgs;

static bool _HorizonProbabilityPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {

	double p = ((HorizonProbabilityPolicyArgs*)(policy_state->args))->p;
	unsigned int k = ((HorizonProbabilityPolicyArgs*)(policy_state->args))->k;
	message_copy* first = ((message_copy*)getCopies(p_msg)->head->data);
	unsigned char hops = 0;
	if(!query_context_header(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "hops", &hops, myID, 0))
	   assert(false);

	double u = randomProb();

	return hops < k || u <= p;
}

static void _HorizonProbabilityPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* HorizonProbabilityPolicy(double p, unsigned int k) {
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = malloc(sizeof(HorizonProbabilityPolicyArgs));
	HorizonProbabilityPolicyArgs* args = ((HorizonProbabilityPolicyArgs*)(r_policy->policy_state.args));
	args->p = p;
	args->k = k;

	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_HorizonProbabilityPolicy;
    r_policy->destroy = &_HorizonProbabilityPolicyDestroy;

	return r_policy;
}
