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

#include <assert.h>

static bool _HopCountAidedPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {

	int hops = -1;

	for(double_list_item* it = getCopies(p_msg)->head; it; it = it->next) {
		message_copy* copy = (message_copy*)it->data;

		unsigned char current_hops = 0;
		if(!query_context_header(r_context, getContextHeader(copy), getBcastHeader(copy)->context_length, "hops", &current_hops, myID, 0))
			assert(false);

		if(hops == -1)
			hops = current_hops;
		else {
			if( current_hops > hops )
				return false;
		}
	}

	return true;
}

RetransmissionPolicy* HopCountAidedPolicy() {
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = NULL;
	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_HopCountAidedPolicy;
    r_policy->destroy = NULL;
    
	return r_policy;
}
