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

#include "retransmission_context_private.h"

#include <assert.h>

static void HopsContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
	// Do nothing
}

static void HopsContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	// Do nothing
}

static bool HopsContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	return false;
}

static bool HopsContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

	if(strcmp(query, "hops") == 0) {
        if(context_header) {
            *((unsigned char*)result) = *((unsigned char*)context_header);
        } else {
            *((unsigned char*)result) = 0; // Request
        }

		return true;
	}

	return false;
}

static unsigned int HopsContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	unsigned int size = sizeof(unsigned char);
	*context_header = malloc(size);

    double_list* copies = getCopies(p_msg);

    assert(copies->size > 0);

	/*if(copies->size == 0) { // Broadcast Request
		*((unsigned char*)*context_header) = 0;
	} else {*/
		message_copy* msg_copy = (message_copy*)copies->head->data;

		unsigned char hops = 0;
		if(!query_context_header(r_context, getContextHeader(msg_copy), getBcastHeader(msg_copy)->context_length, "hops", &hops, myID, 0))
			assert(false);

        hops++;
        memcpy(*context_header, &hops, size);
	//}

	return size;
}

RetransmissionContext* HopsContext() {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

	r_context->context_state.args = NULL;
    r_context->context_state.vars = NULL;

	r_context->init = &HopsContextInit;
	r_context->create_header = &HopsContextHeader;
	r_context->process_event = &HopsContextEvent;
	r_context->query_handler = &HopsContextQuery;
	r_context->query_header_handler = &HopsContextQueryHeader;
    r_context->copy_handler = NULL;
    r_context->destroy = NULL;

	return r_context;
}
