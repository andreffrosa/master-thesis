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

static bool HorizonProbabilityPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {

	double p = ((HorizonProbabilityPolicyArgs*)(policy_state->args))->p;
	unsigned int k = ((HorizonProbabilityPolicyArgs*)(policy_state->args))->k;
	MessageCopy* first = ((MessageCopy*)getCopies(p_msg)->head->data);
	unsigned char hops = 0;

    list* visited2 = list_init();
    if(!RC_queryHeader(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "hops", &hops, NULL, myID, visited2)) {
        assert(false);
    }
    list_delete(visited2);

	double u = randomProb();
	return hops < k || u <= p;
}

static void HorizonProbabilityPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* HorizonProbabilityPolicy(double p, unsigned int k) {

	HorizonProbabilityPolicyArgs* args = malloc(sizeof(HorizonProbabilityPolicyArgs));
	args->p = p;
	args->k = k;

    return newRetransmissionPolicy(
        args,
        NULL,
        &HorizonProbabilityPolicyEval,
        &HorizonProbabilityPolicyDestroy
    );
}
