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

#include "utility/my_math.h"

static void ParentsContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
	// Do nothing
}

static void ParentsContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	// Do nothing
}

static bool ParentsContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	return false;
}

static bool ParentsContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	unsigned int amount = context_header_size / sizeof(uuid_t);

	if(strcmp(query, "parents") == 0) {
		list* l = list_init();

		for(int i = 0; i < amount; i++) {
			unsigned char* id = malloc(sizeof(uuid_t));
			unsigned char* ptr = context_header + i*sizeof(uuid_t);
			uuid_copy(id, ptr);
			list_add_item_to_tail(l, id);
		}

        *((list**)result) = l;
		return true;
	}

	return false;
}

static unsigned int ParentsContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    double_list* copies = getCopies(p_msg);
    assert(copies->size > 0);
    /*if(copies->size == 0) {
        *context_header = NULL;
        return 0;
    }*/

    unsigned int max_amount = *((unsigned int*)(context_state->args));
    unsigned int real_amount = iMin(max_amount, copies->size);
    unsigned int size = real_amount*sizeof(uuid_t);

	unsigned char* buffer = malloc(size);

	double_list_item* it = copies->head;
	for(int i = 0; i < real_amount; i++, it = it->next) {
		message_copy* msg_copy = (message_copy*)it->data;
		unsigned char* ptr = (buffer + i*sizeof(uuid_t));
		uuid_copy(ptr, getBcastHeader(msg_copy)->sender_id);
	}

    *context_header = buffer;
	return size;
}

static void ParentsContextDestroy(ModuleState* context_state, list* visited) {
    free(context_state->args);
}

RetransmissionContext* ParentsContext(unsigned int max_amount) {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

	r_context->context_state.args = malloc(sizeof(unsigned int));
	*((unsigned int*)(r_context->context_state.args)) = max_amount;
	r_context->context_state.vars = NULL;

	r_context->init = &ParentsContextInit;
	r_context->create_header = &ParentsContextHeader;
	r_context->process_event = &ParentsContextEvent;
	r_context->query_handler = &ParentsContextQuery;
	r_context->query_header_handler = &ParentsContextQueryHeader;
    r_context->copy_handler = NULL;
    r_context->destroy = &ParentsContextDestroy;

	return r_context;
}
