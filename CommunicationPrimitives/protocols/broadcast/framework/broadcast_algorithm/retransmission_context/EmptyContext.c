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

static void EmptyContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
	// Do nothing
}

static unsigned int EmptyContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	*context_header = NULL;
	return 0;
}

static void EmptyContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	// Do nothing
}

static bool EmptyContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	return false;
}

static bool EmptyContextQueryHeader(ModuleState* context_state, void* header, unsigned int header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	return false;
}

RetransmissionContext* EmptyContext() {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

	r_context->context_state.args = NULL;
    r_context->context_state.vars = NULL;

	r_context->init = &EmptyContextInit;
	r_context->create_header = &EmptyContextHeader;
	r_context->process_event = &EmptyContextEvent;
	r_context->query_handler = &EmptyContextQuery;
	r_context->query_header_handler = &EmptyContextQueryHeader;
    r_context->copy_handler = NULL;
    r_context->destroy = NULL;

	return r_context;
}
