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

static unsigned long HopCountAwareRADExtensionDelayCompute(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, hash_table* contexts) {

    MessageCopy* first = ((MessageCopy*)getCopies(p_msg)->head->data);
    hash_table* headers = getHeaders(first);

    unsigned long* parent_initial_delay = (unsigned long*)hash_table_find_value(headers, "delay");
    assert(parent_initial_delay);

    unsigned long delta_t = *((unsigned long*) (delay_state->args));

    if(!isCopy) {
        	double u = randomProb();
        	return (unsigned long) roundl(u*delta_t) + (delta_t - *parent_initial_delay);
    } else {
        return remaining + 2*delta_t;
    }
}

static void HopCountAwareRADExtensionDelayDestroy(ModuleState* delay_state) {
    free(delay_state->args);
}

// T_max = 2*delta_T*(Cth-1); Cth = counter threshold
RetransmissionDelay* HopCountAwareRADExtensionDelay(unsigned long delta_t) {

    unsigned long* delta_t_args = malloc(sizeof(delta_t));
    *delta_t_args = delta_t;

    RetransmissionDelay* r_delay = newRetransmissionDelay(
        delta_t_args,
        NULL,
        &HopCountAwareRADExtensionDelayCompute,
        &HopCountAwareRADExtensionDelayDestroy,
        new_list(1, new_str("HopCountAwareRADExtensionContext"))
    );

    return r_delay;
}
