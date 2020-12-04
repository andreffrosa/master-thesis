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

static bool NeighborCountingPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
	unsigned int c = *((unsigned int*)(policy_state->args));
	unsigned int n_copies = getCopies(p_msg)->size;
	unsigned int n = 0;

    list* visited2 = list_init();
    if(!RC_query(r_context, "in_degree", &n, NULL, myID, visited2))
        assert(false);
    list_delete(visited2);

	return n_copies < lMin(n, c);
}

static void NeighborCountingPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* NeighborCountingPolicy(unsigned int c) {

    unsigned int* c_arg = malloc(sizeof(c));
    *c_arg = c;

    return newRetransmissionPolicy(
        c_arg,
        NULL,
        &NeighborCountingPolicyEval,
        &NeighborCountingPolicyDestroy
    );
}
