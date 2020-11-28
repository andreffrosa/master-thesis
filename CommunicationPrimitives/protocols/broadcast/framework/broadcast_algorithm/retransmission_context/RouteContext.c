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

#include "utility/my_math.h"

static void RouteContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
	// Do nothing
}

static void RouteContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	// Do nothing
}

static bool RouteContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	return false;
}

static bool RouteContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

	if(strcmp(query, "route") == 0 || strcmp(query, "path") == 0) {
        unsigned int amount = context_header_size / sizeof(uuid_t);

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

static unsigned int RouteContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    double_list* copies = getCopies(p_msg);
    assert(copies->size > 0);
    /*if(copies->size == 0) {
        *context_header = NULL;
        return 0;
    }*/

    message_copy* first = (message_copy*)copies->head->data;

    list* route = NULL;
    if(!query_context_header(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "route", &route, myID, 0))
		assert(false);

    unsigned char* x = malloc(sizeof(uuid_t));
    uuid_copy(x, myID);
    list_add_item_to_tail(route, x);

    unsigned int max_len = *((unsigned int*)(context_state->args));
    unsigned int real_len = iMin(max_len, route->size);

    while (route->size > max_len) {
        free(list_remove_head(route));
    }

    unsigned int size = real_len*sizeof(uuid_t);

    unsigned char* buffer = malloc(size);
    unsigned char* ptr = buffer;
    for(list_item* it = route->head; it; it = it->next) {
        unsigned char* id = (unsigned char*)(it->data);
        memcpy(ptr, id, sizeof(uuid_t));
        ptr += sizeof(uuid_t);
    }

    *context_header = buffer;
    return size;
}

static void RouteContextDestroy(ModuleState* context_state, list* visited) {
    free(context_state->args);
}

RetransmissionContext* RouteContext(unsigned int max_len) {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

	r_context->context_state.args = malloc(sizeof(unsigned int));
	*((unsigned int*)(r_context->context_state.args)) = max_len;
	r_context->context_state.vars = NULL;

	r_context->init = &RouteContextInit;
	r_context->create_header = &RouteContextHeader;
	r_context->process_event = &RouteContextEvent;
	r_context->query_handler = &RouteContextQuery;
	r_context->query_header_handler = &RouteContextQueryHeader;
    r_context->copy_handler = NULL;
    r_context->destroy = &RouteContextDestroy;

	return r_context;
}
