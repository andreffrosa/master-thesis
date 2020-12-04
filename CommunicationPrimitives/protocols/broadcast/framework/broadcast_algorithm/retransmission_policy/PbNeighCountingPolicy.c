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

static bool PbNeighCountingPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID,RetransmissionContext* r_context, list* visited) {
	unsigned int c1 = ((unsigned int*)(policy_state->args))[0], c2 = ((unsigned int*)(policy_state->args))[1];
	unsigned int n_copies = getCopies(p_msg)->size;
	unsigned int n = 0;

    list* visited2 = list_init();
    if(!RC_query(r_context, "in_degree", &n, NULL, myID, visited2))
        assert(false);
    list_delete(visited2);

	if(n_copies < n) {
		if(n_copies <= c1) {
			return true;
		} else if(n_copies < c2) {
			double p = (c2 - n_copies) / ((double) (c2 - c1)); // Probability of retransmitting
			return (randomProb() <= p);
		}
	}

	return false;
}

static void PbNeighCountingPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* PbNeighCountingPolicy(unsigned int c1, unsigned int c2) {
    unsigned int* c_args = malloc(2*sizeof(unsigned int));
    c_args[0] = c1;
    c_args[1] = c2;

    return newRetransmissionPolicy(
        c_args,
        NULL,
        &PbNeighCountingPolicyEval,
        &PbNeighCountingPolicyDestroy
    );
}
