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

#include "retransmission_context_private.h"

#include <assert.h>

static bool HopCountAwareRADExtensionContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {

	if(strcmp(query, "delay") == 0 || strcmp(query, "timer") == 0 || strcmp(query, "rad") == 0) {
		*((unsigned long*)result) = context_header ? *((unsigned long*)context_header) : 0;
		return true;
	}

	return false;
}

static unsigned int HopCountAwareRADExtensionContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, unsigned char* myID, list* visited) {

    unsigned long delta_t = *((unsigned long*) (context_state->args));

    double_list* copies = getCopies(p_msg);
    unsigned char n_copies = copies->size;

    assert(n_copies > 0);

    //if(n_copies > 0) {
        unsigned int size = sizeof(unsigned long);
    	unsigned char* buffer = malloc(size);

        unsigned long delay = getCurrentPhaseDuration(p_msg);

        unsigned long parent_initial_delay;
        MessageCopy* first = ((MessageCopy*)copies->head->data);

        if(!HopCountAwareRADExtensionContextQueryHeader(context_state, getContextHeader(first), getBcastHeader(first)->context_length, "delay", &parent_initial_delay, NULL, myID, NULL))
            assert(false);

        unsigned long initial_delay = delay - 2*n_copies*delta_t - (delta_t - parent_initial_delay);
        memcpy(buffer, &initial_delay, sizeof(unsigned long));

        *context_header = buffer;
    	return size;
    /*} else {
        *context_header = NULL;
        return 0;
    }*/
}

static void HopCountAwareRADExtensionContextDestroy(ModuleState* context_state, list* visited) {
    free(context_state->args);
}

RetransmissionContext* HopCountAwareRADExtensionContext(unsigned long delta_t) {
    assert(delta_t > 0);

    unsigned long* delta_t_arg = malloc(sizeof(unsigned long));
    *delta_t_arg = delta_t;

    return newRetransmissionContext(
        delta_t_arg,
        NULL,
        NULL,
        NULL,
        &HopCountAwareRADExtensionContextHeader,
        NULL,
        &HopCountAwareRADExtensionContextQueryHeader,
        NULL,
        &HopCountAwareRADExtensionContextDestroy
    );
}
