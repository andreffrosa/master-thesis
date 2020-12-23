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

static bool CountPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {
	unsigned int c = *((unsigned int*)(policy_state->args));
	unsigned int n_copies = getCopies(p_msg)->size;

	return n_copies < c;
}

static void CountPolicyDestroy(ModuleState* policy_state) {
    free(policy_state->args);
}

RetransmissionPolicy* CountPolicy(unsigned int c) {

    unsigned int* c_arg = malloc(sizeof(c));
    *c_arg = c;

    return newRetransmissionPolicy(
        c_arg,
        NULL,
        &CountPolicyEval,
        &CountPolicyDestroy,
        NULL
    );
}
