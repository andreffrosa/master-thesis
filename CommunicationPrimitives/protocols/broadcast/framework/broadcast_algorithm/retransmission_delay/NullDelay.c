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

static unsigned long _NullDelay(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, RetransmissionContext* r_context, unsigned char* myID) {
	return 0L;
}

RetransmissionDelay* NullDelay() {
    RetransmissionDelay* r_delay = malloc(sizeof(RetransmissionDelay));

	r_delay->delay_state.args = NULL;
	r_delay->delay_state.vars = NULL;
	r_delay->r_delay = &_NullDelay;
    r_delay->destroy = NULL;

	return r_delay;
}
