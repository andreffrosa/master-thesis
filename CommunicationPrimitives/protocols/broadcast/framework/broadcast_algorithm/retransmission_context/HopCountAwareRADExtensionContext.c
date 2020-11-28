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

static void HopCountAwareRADExtensionContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
	// Do nothing
}

static void HopCountAwareRADExtensionContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	// Do nothing
}

static bool HopCountAwareRADExtensionContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	return false;
}

static bool HopCountAwareRADExtensionContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

	if(strcmp(query, "delay") == 0 || strcmp(query, "timer") == 0 || strcmp(query, "rad") == 0) {
		*((unsigned long*)result) = context_header ? *((unsigned long*)context_header) : 0;
		return true;
	}

	return false;
}

static unsigned int HopCountAwareRADExtensionContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

    unsigned long delta_t = *((unsigned long*) (context_state->args));

    double_list* copies = getCopies(p_msg);
    unsigned char n_copies = copies->size;

    assert(n_copies > 0);

    //if(n_copies > 0) {
        unsigned int size = sizeof(unsigned long);
    	unsigned char* buffer = malloc(size);

        unsigned long delay = getCurrentPhaseDuration(p_msg);

        unsigned long parent_initial_delay;
        message_copy* first = ((message_copy*)copies->head->data);
        if(!query_context_header(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "delay", &parent_initial_delay, myID, 0))
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
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

    r_context->context_state.args = malloc(sizeof(delta_t));
	*((unsigned long*)(r_context->context_state.args)) = delta_t;

    r_context->context_state.vars = NULL;

	r_context->init = &HopCountAwareRADExtensionContextInit;
	r_context->create_header = &HopCountAwareRADExtensionContextHeader;
	r_context->process_event = &HopCountAwareRADExtensionContextEvent;
	r_context->query_handler = &HopCountAwareRADExtensionContextQuery;
	r_context->query_header_handler = &HopCountAwareRADExtensionContextQueryHeader;
    r_context->copy_handler = NULL;
    r_context->destroy = &HopCountAwareRADExtensionContextDestroy;

	return r_context;
}
