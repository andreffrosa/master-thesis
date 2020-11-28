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

bool query_context(RetransmissionContext* r_context, char* query, void* result, unsigned char* myID, int argc, ...) {
	va_list args;
	va_start(args, argc);

    list* visited = list_init();
    void** this = malloc(sizeof(void*));
    *this = r_context;
    list_add_item_to_tail(visited, this);

	bool r = r_context->query_handler(&r_context->context_state, query, result, argc, &args, r_context, myID, visited);

    list_delete(visited);

	va_end(args);
	return r;
}

bool query_context_header(RetransmissionContext* r_context, void* context_header, unsigned int context_length, char* query, void* result, unsigned char* myID, int argc, ...) {
	va_list args;
	va_start(args, argc);

    list* visited = list_init();
    void** this = malloc(sizeof(void*));
    *this = r_context;
    list_add_item_to_tail(visited, this);

	bool r = r_context->query_header_handler(&r_context->context_state, context_header, context_length, query, result, argc, &args, r_context, myID, visited);

    list_delete(visited);

	va_end(args);
	return r;
}

void destroyRetransmissionContext(RetransmissionContext* context, list* visited) {
    if(context !=NULL) {
        if(context->destroy != NULL) {
            bool root = visited == NULL;
            if(root) {
                visited = list_init();
                void** this = malloc(sizeof(void*));
                *this = context;
                list_add_item_to_tail(visited, this);
            }

            context->destroy(&context->context_state, visited);

            if(root) {
                list_delete(visited);
            }
        }
        free(context);
    }
}
