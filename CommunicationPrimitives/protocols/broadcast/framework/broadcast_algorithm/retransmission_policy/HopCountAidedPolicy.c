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

static bool HopCountAidedPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {

	int hops = -1;

	for(double_list_item* it = getCopies(p_msg)->head; it; it = it->next) {
		MessageCopy* copy = (MessageCopy*)it->data;

		unsigned char current_hops = 0;

        list* visited2 = list_init();
        if(!RC_queryHeader(r_context, getContextHeader(copy), getBcastHeader(copy)->context_length, "hops", &current_hops, NULL, myID, visited2)) {
            assert(false);
        }
        list_delete(visited2);

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
	return newRetransmissionPolicy(
        NULL,
        NULL,
        &HopCountAidedPolicyEval,
        NULL
    );
}
