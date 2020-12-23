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

static bool HopCountAidedPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {

	int hops = -1;

	for(double_list_item* it = getCopies(p_msg)->head; it; it = it->next) {
		MessageCopy* copy = (MessageCopy*)it->data;
        hash_table* headers = getHeaders(copy);

        byte* current_hops = (byte*)hash_table_find_value(headers, "hops");
        assert(current_hops);

		if(hops == -1)
			hops = *current_hops;
		else {
			if( *current_hops > hops )
				return false;
		}
	}

	return true;
}

RetransmissionPolicy* HopCountAidedPolicy() {
	RetransmissionPolicy* r_policy = newRetransmissionPolicy(
        NULL,
        NULL,
        &HopCountAidedPolicyEval,
        NULL,
        new_list(1, new_str("HopsContext"))
    );

    return r_policy;
}
