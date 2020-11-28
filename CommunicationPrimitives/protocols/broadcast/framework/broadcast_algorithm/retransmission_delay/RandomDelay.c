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

static unsigned long _RandomDelay(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, RetransmissionContext* r_context, unsigned char* myID) {
    if(!isCopy) {
        	unsigned long t_max = *((unsigned long*) (delay_state->args));
        	double u = randomProb();
        	return (unsigned long) roundl(u*t_max);
    } else {
        return remaining;
    }
}

static void _RandomDelayDestroy(ModuleState* delay_state, list* visited) {
    free(delay_state->args);
}

RetransmissionDelay* RandomDelay(unsigned long t_max) {
    RetransmissionDelay* r_delay = malloc(sizeof(RetransmissionDelay));

	r_delay->delay_state.args = malloc(sizeof(t_max));
	*((unsigned long*)(r_delay->delay_state.args)) = t_max;

	r_delay->delay_state.vars = NULL;

	r_delay->r_delay = &_RandomDelay;
    r_delay->destroy = &_RandomDelayDestroy;

	return r_delay;
}
