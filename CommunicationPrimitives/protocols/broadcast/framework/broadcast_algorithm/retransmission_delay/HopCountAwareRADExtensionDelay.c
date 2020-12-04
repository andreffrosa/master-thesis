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

static unsigned long HopCountAwareRADExtensionDelayCompute(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, RetransmissionContext* r_context, list* visited) {

    unsigned long parent_initial_delay;
    MessageCopy* first = ((MessageCopy*)getCopies(p_msg)->head->data);

    list* visited2 = list_init();
    if(!RC_queryHeader(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "delay", &parent_initial_delay, NULL, myID, visited2))
        assert(false);
    list_delete(visited2);

    unsigned long delta_t = *((unsigned long*) (delay_state->args));

    if(!isCopy) {
        	double u = randomProb();
        	return (unsigned long) roundl(u*delta_t) + (delta_t - parent_initial_delay);
    } else {
        return remaining + 2*delta_t;
    }
}

static void HopCountAwareRADExtensionDelayDestroy(ModuleState* delay_state, list* visited) {
    free(delay_state->args);
}

// T_max = 2*delta_T*(Cth-1); Cth = counter threshold
RetransmissionDelay* HopCountAwareRADExtensionDelay(unsigned long delta_t) {

    unsigned long* delta_t_args = malloc(sizeof(delta_t));
    *delta_t_args = delta_t;

    return newRetransmissionDelay(
        delta_t_args,
        NULL,
        &HopCountAwareRADExtensionDelayCompute,
        &HopCountAwareRADExtensionDelayDestroy
    );
}
