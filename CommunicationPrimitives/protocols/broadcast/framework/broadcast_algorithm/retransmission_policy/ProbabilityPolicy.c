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

static bool _ProbabilityPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {
	double p = *((double*)(policy_state->args));
	double u = randomProb();

	return u <= p;
}

static void _ProbabilityPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* ProbabilityPolicy(double p) {
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = malloc(sizeof(p));
	*((double*)(r_policy->policy_state.args)) = p;
	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_ProbabilityPolicy;
    r_policy->destroy = &_ProbabilityPolicyDestroy;

	return r_policy;
}
