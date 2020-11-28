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

static unsigned long _TwoPhaseRandomDelay(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, RetransmissionContext* r_context, unsigned char* myID) {
    if(!isCopy) {
        unsigned long t1 = ((unsigned long*)(delay_state->args))[0];
        unsigned long t2 = ((unsigned long*)(delay_state->args))[1];
        unsigned int current_phase = getCurrentPhase(p_msg);
        double u = randomProb();

        return (unsigned long)(current_phase == 1 ? roundl(u*t1) : roundl(u*t2));
    } else {
        return remaining;
    }
}

static void _TwoPhaseRandomDelayDestroy(ModuleState* delay_state, list* visited) {
    free(delay_state->args);
}

RetransmissionDelay* TwoPhaseRandomDelay(unsigned long t1, unsigned long t2) {
    RetransmissionDelay* r_delay = malloc(sizeof(RetransmissionDelay));

	r_delay->delay_state.args = malloc(2*sizeof(unsigned long));
	unsigned long* t = ((unsigned long*)(r_delay->delay_state.args));
	t[0] = t1;
	t[1] = t2;

	r_delay->delay_state.vars = NULL;
	r_delay->r_delay = &_TwoPhaseRandomDelay;
    r_delay->destroy = &_TwoPhaseRandomDelayDestroy;

	return r_delay;
}
