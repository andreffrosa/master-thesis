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

#include "utility/my_math.h"

#include <assert.h>

static unsigned long _HopCountAwareRADExtensionDelay(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, RetransmissionContext* r_context, unsigned char* myID) {

    unsigned long parent_initial_delay;
    message_copy* first = ((message_copy*)getCopies(p_msg)->head->data);
    if(!query_context_header(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "delay", &parent_initial_delay, myID, 0))
		assert(false);

    unsigned long delta_t = *((unsigned long*) (delay_state->args));

    if(!isCopy) {
        	double u = randomProb();
        	return (unsigned long) roundl(u*delta_t) + (delta_t - parent_initial_delay);
    } else {
        return remaining + 2*delta_t;
    }
}

static void _HopCountAwareRADExtensionDelayDestroy(ModuleState* delay_state, list* visited) {
    free(delay_state->args);
}

// T_max = 2*delta_T*(Cth-1); Cth = counter threshold
RetransmissionDelay* HopCountAwareRADExtensionDelay(unsigned long delta_t) {
    RetransmissionDelay* r_delay = malloc(sizeof(RetransmissionDelay));

	r_delay->delay_state.args = malloc(sizeof(delta_t));
	*((unsigned long*)(r_delay->delay_state.args)) = delta_t;

	r_delay->delay_state.vars = NULL;
	r_delay->r_delay = &_HopCountAwareRADExtensionDelay;
    r_delay->destroy = &_HopCountAwareRADExtensionDelayDestroy;

	return r_delay;
}
