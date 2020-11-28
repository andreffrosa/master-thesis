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

static bool _RapidPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {
	double beta = *((double*)(policy_state->args));
	unsigned int n = 0;
	if(!query_context(r_context, "n_neighbors", &n, myID, 0))
		assert(false);

	double p  = dMin(1.0, (beta / n));
	double u = randomProb();

	return u <= p;
}

static void _AHBPPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* RapidPolicy(double beta) {
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = malloc(sizeof(double));
	*((double*)(r_policy->policy_state.args)) = beta;

	r_policy->policy_state.vars = NULL;
	r_policy->r_policy = &_RapidPolicy;
    r_policy->destroy = &_AHBPPolicyDestroy;

	return r_policy;
}
