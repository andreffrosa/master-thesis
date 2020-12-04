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

static unsigned long RandomDelayCompute(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
    if(!isCopy) {
        	unsigned long t_max = *((unsigned long*) (delay_state->args));
        	double u = randomProb();
        	return (unsigned long) roundl(u*t_max);
    } else {
        return remaining;
    }
}

static void RandomDelayDestroy(ModuleState* delay_state, list* visited) {
    free(delay_state->args);
}

RetransmissionDelay* RandomDelay(unsigned long t_max) {

    unsigned long* t_max_arg = malloc(sizeof(t_max));
    *t_max_arg = t_max;

    return newRetransmissionDelay(
        t_max_arg,
        NULL,
        &RandomDelayCompute,
        &RandomDelayDestroy
    );
}
