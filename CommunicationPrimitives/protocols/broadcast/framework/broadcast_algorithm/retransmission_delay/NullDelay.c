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

#include "retransmission_delay_private.h"

static unsigned long NullDelayCompute(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, hash_table* contexts) {
	return 0L;
}

RetransmissionDelay* NullDelay() {
    return newRetransmissionDelay(
        NULL,
        NULL,
        &NullDelayCompute,
        NULL,
        NULL
    );
}
