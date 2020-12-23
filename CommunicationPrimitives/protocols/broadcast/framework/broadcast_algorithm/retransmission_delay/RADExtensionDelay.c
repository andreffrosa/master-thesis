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
 * (C) 2020
 *********************************************************/

#include "retransmission_delay_private.h"

#include "utility/my_math.h"

static unsigned long RADExtensionDelayCompute(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, hash_table* contexts) {

    unsigned long delta_t = *((unsigned long*) (delay_state->args));
    if(!isCopy) {
        	double u = randomProb();
        	return (unsigned long) roundl(u*delta_t);
    } else {
        return remaining + delta_t;
    }
}

static void RADExtensionDelayDestroy(ModuleState* delay_state) {
    free(delay_state->args);
}

// T_max = delta_T*(Cth-1); Cth = counter threshold
RetransmissionDelay* RADExtensionDelay(unsigned long delta_t) {

    unsigned long* delta_t_arg = malloc(sizeof(delta_t));
	*delta_t_arg = delta_t;

    return newRetransmissionDelay(
        delta_t_arg,
        NULL,
        &RADExtensionDelayCompute,
        &RADExtensionDelayDestroy,
        NULL
    );
}
