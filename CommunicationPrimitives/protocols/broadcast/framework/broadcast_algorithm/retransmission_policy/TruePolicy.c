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
 * (C) 2019
 *********************************************************/

#include "retransmission_policy_private.h"

static bool TruePolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
	return true;
}

RetransmissionPolicy* TruePolicy() {
    return newRetransmissionPolicy(
        NULL,
        NULL,
        &TruePolicyEval,
        NULL
    );
}
