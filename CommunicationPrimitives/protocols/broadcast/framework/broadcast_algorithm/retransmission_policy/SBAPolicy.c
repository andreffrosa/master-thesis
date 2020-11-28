/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Author:
 * André Rosa (af.rosa@campus.fct.unl.pt)
 * Under the guidance of:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2020
 *********************************************************/

#include "retransmission_policy_private.h"

#include <assert.h>

static bool _SBAPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {

    bool all_covered = false;
    if(!query_context(r_context, "all_covered", &all_covered, myID, 1, p_msg))
        assert(false);

    return !all_covered;
}

RetransmissionPolicy* SBAPolicy() {
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = NULL;
	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_SBAPolicy;
    r_policy->destroy = NULL;

	return r_policy;
}
