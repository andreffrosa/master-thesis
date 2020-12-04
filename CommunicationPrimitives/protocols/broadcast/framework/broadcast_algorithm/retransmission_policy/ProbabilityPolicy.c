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

static bool ProbabilityPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
	double p = *((double*)(policy_state->args));
	double u = randomProb();

	return u <= p;
}

static void ProbabilityPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* ProbabilityPolicy(double p) {

	double* p_arg = malloc(sizeof(p));
	*p_arg = p;

    return newRetransmissionPolicy(
        p_arg,
        NULL,
        &ProbabilityPolicyEval,
        &ProbabilityPolicyDestroy
    );
}
