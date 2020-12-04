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

RetransmissionContext* newRetransmissionContext(void* args, void* vars, init_ctx init, ctx_event_handler process_event, create_ctx_header create_header, ctx_query_handler query_handler, ctx_query_header_handler query_header_handler, ctx_copy_handler copy_handler, destroy_ctx destroy) {
    RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

	r_context->state.args = args;
	r_context->state.vars = vars;

	r_context->init = init;
	r_context->create_header = create_header;
	r_context->process_event = process_event;
	r_context->query_handler = query_handler;
	r_context->query_header_handler = query_header_handler;
    r_context->copy_handler = copy_handler;
    r_context->destroy = destroy;

	return r_context;
}

void RC_init(RetransmissionContext* context, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    assert(context && visited);

    if( list_find_item(visited, &equalAddr, context) == NULL ) {
        void** this = malloc(sizeof(void*));
        *this = context;
        list_add_item_to_tail(visited, this);

        if( context->init ) {
            context->init(&context->state, protocol_definition, myID, visited);
        }
    }

}

void RC_processEvent(RetransmissionContext* context, queue_t_elem* elem, unsigned char* myID, list* visited) {
    assert(context && visited);

    if( list_find_item(visited, &equalAddr, context) == NULL ) {
        void** this = malloc(sizeof(void*));
        *this = context;
        list_add_item_to_tail(visited, this);

        if( context->process_event ) {
            context->process_event(&context->state, elem, myID, visited);
        }
    }
}

unsigned int RC_createHeader(RetransmissionContext* context, PendingMessage* p_msg, void** context_header, unsigned char* myID, list* visited) {
    assert(context && visited);

    if( list_find_item(visited, &equalAddr, context) == NULL ) {
        void** this = malloc(sizeof(void*));
        *this = context;
        list_add_item_to_tail(visited, this);

        if( context->create_header ) {
            return context->create_header(&context->state, p_msg, context_header, myID, visited);
        } else {
            *context_header = NULL;
            return 0;
        }
    } else {
        *context_header = NULL;
        return 0;
        // TODO: what should be done?
    }
}

bool RC_query(RetransmissionContext* context, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {
    assert(context && visited && result);

    if( list_find_item(visited, &equalAddr, context) == NULL ) {
        void** this = malloc(sizeof(void*));
        *this = context;
        list_add_item_to_tail(visited, this);

        if( context->query_handler ) {
            return context->query_handler(&context->state, query, result, query_args, myID, visited);
        } else {
            //*result = NULL;
            return false;
        }
    } else {
        //*result = NULL;
        return false;
    }
}

bool RC_queryHeader(RetransmissionContext* context, void* context_header, unsigned int context_header_size, const char* query, void* result, hash_table* args, unsigned char* myID, list* visited) {
    assert(context && visited && result);

    if( list_find_item(visited, &equalAddr, context) == NULL ) {
        void** this = malloc(sizeof(void*));
        *this = context;
        list_add_item_to_tail(visited, this);

        if( context->query_header_handler ) {
            return context->query_header_handler(&context->state, context_header, context_header_size, query, result, args, myID, visited);
        } else {
            //*result = NULL;
            return false;
        }
    } else {
        //*result = NULL;
        return false;
    }
}

void RC_processCopy(RetransmissionContext* context, PendingMessage* p_msg, unsigned char* myID, list* visited) {
    assert(context && visited);

    if( list_find_item(visited, &equalAddr, context) == NULL ) {
        void** this = malloc(sizeof(void*));
        *this = context;
        list_add_item_to_tail(visited, this);

        if( context->copy_handler ) {
            context->copy_handler(&context->state, p_msg, myID, visited);
        }
    }
}

void destroyRetransmissionContext(RetransmissionContext* context, list* visited) {
    assert(visited);

    if(context != NULL) {
        if( list_find_item(visited, &equalAddr, context) == NULL ) {
            void** this = malloc(sizeof(void*));
            *this = context;
            list_add_item_to_tail(visited, this);

            if(context->destroy != NULL) {
                context->destroy(&context->state, visited);
            }
            free(context);
        }
    }
}





/*


bool query_context(RetransmissionContext* r_context, char* query, void* result, unsigned char* myID, int argc, ...) {
	va_list args;
	va_start(args, argc);

    list* visited = list_init();
    void** this = malloc(sizeof(void*));
    *this = r_context;
    list_add_item_to_tail(visited, this);

	bool r = r_context->query_handler(&r_context->state, query, result, argc, &args, r_context, myID, visited);

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

	bool r = r_context->query_header_handler(&r_context->state, context_header, context_length, query, result, argc, &args, r_context, myID, visited);

    list_delete(visited);

	va_end(args);
	return r;
}



*/
