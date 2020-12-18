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

static bool RapidPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
	double beta = *((double*)(policy_state->args));
	unsigned int n = 0;

    list* visited2 = list_init();
    if(!RC_query(r_context, "n_neighbors", &n, NULL, myID, visited2))
		assert(false);
    list_delete(visited2);

	double p  = dMin(1.0, (beta / n));
	double u = randomProb();

	return u <= p;
}

static void RapidPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* RapidPolicy(double beta) {

	double* beta_arg = malloc(sizeof(double));
	*beta_arg = beta;

    return newRetransmissionPolicy(
        beta_arg,
        NULL,
        &RapidPolicyEval,
        &RapidPolicyDestroy
    );
}