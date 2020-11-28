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

#include <stdarg.h>
#include <assert.h>

typedef struct _ComposeContextArgs {
	RetransmissionContext** contexts;
	unsigned int amount;
} ComposeContextArgs;

static void ComposeContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
	ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));

	for(int i = 0; i < args->amount; i++) {
        if(list_find_item(visited, &equalAddr, args->contexts[i]) == NULL) {
            void** this = malloc(sizeof(void*));
            *this = args->contexts[i];
            list_add_item_to_tail(visited, this);

            args->contexts[i]->init(&args->contexts[i]->context_state, protocol_definition, myID, visited);
        }
	}
}

static unsigned int ComposeContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

	ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));

	unsigned char sizes[args->amount];
    void* headers[args->amount];
    unsigned int total_size = sizeof(sizes);

    for(int i = 0; i < args->amount; i++) {
        if(list_find_item(visited, &equalAddr, args->contexts[i]) == NULL) {
            void** this = malloc(sizeof(void*));
            *this = args->contexts[i];
            list_add_item_to_tail(visited, this);

            unsigned int temp = args->contexts[i]->create_header(&args->contexts[i]->context_state, p_msg, &headers[i], r_context, myID, visited);
            assert(temp <= 255);
            sizes[i] = temp;
        } else {
            sizes[i] = 0;
            headers[i] = NULL;
        }

        total_size += sizes[i];
    }

	unsigned char* buffer = malloc(total_size);
    unsigned char* ptr = buffer;
    memcpy(ptr, sizes, sizeof(sizes));
    ptr += sizeof(sizes);
    for(int i = 0; i < args->amount; i++) {
        if(sizes[i] > 0) {
            memcpy(ptr, headers[i], sizes[i]);
            ptr += sizes[i];

            free(headers[i]);
        }
    }

	*context_header = buffer;
	return total_size;
}

static void ComposeContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));

    for(int i = 0; i < args->amount; i++) {
        if(list_find_item(visited, &equalAddr, args->contexts[i]) == NULL) {
            void** this = malloc(sizeof(void*));
            *this = args->contexts[i];
            list_add_item_to_tail(visited, this);

		    args->contexts[i]->process_event(&args->contexts[i]->context_state, elem, r_context, myID, visited);
        }
	}
}

static bool ComposeContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

	ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));
	for(int i = 0; i < args->amount; i++) {
        if(list_find_item(visited, &equalAddr, args->contexts[i]) == NULL) {
            void** this = malloc(sizeof(void*));
            *this = args->contexts[i];
            list_add_item_to_tail(visited, this);

            bool r = args->contexts[i]->query_handler(&args->contexts[i]->context_state, query, result, argc, argv, r_context, myID, visited);
		    if(r) return true;
        }
	}

	return false;
}

static bool ComposeContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

	ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));

	unsigned char sizes[args->amount];
    if(context_header) {
        memcpy(sizes, context_header, sizeof(sizes));
    } else {
        memset(sizes, 0, sizeof(sizes));
    }

    unsigned char* ptr = context_header ? context_header + sizeof(sizes) : NULL;
	for(int i = 0; i < args->amount; i++) {
		if(sizes[i] > 0 && context_header != NULL) {
            if(list_find_item(visited, &equalAddr, args->contexts[i]) == NULL) {
                void** this = malloc(sizeof(void*));
                *this = args->contexts[i];
                list_add_item_to_tail(visited, this);

    			bool r = args->contexts[i]->query_header_handler(&args->contexts[i]->context_state, ptr, sizes[i], query, result, argc, argv, r_context, myID, visited);
    			if(r) return true;
            }
		}

        ptr += sizes[i];
	}
    assert(ptr == context_header + context_header_size);

	return false;
}

static void ComposeContextCopy(ModuleState* context_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));
    for(int i = 0; i < args->amount; i++) {
        if(list_find_item(visited, &equalAddr, args->contexts[i]) == NULL) {
            void** this = malloc(sizeof(void*));
            *this = args->contexts[i];
            list_add_item_to_tail(visited, this);

            RetransmissionContext* rc = args->contexts[i];
            if(rc->copy_handler != NULL) {
                if(rc->copy_handler != NULL)
                    rc->copy_handler(&rc->context_state, p_msg, r_context, myID, visited);
            }
        }
    }
}

static void ComposeContextDestroy(ModuleState* context_state, list* visited) {
    ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));

    for(int i = 0; i < args->amount; i++) {
        if(list_find_item(visited, &equalAddr, args->contexts[i]) == NULL) {
            void** this = malloc(sizeof(void*));
            *this = args->contexts[i];
            list_add_item_to_tail(visited, this);

            destroyRetransmissionContext(args->contexts[i], visited);
        }
    }

    free(args->contexts);
    free(args);
}

RetransmissionContext* ComposeContext(int amount, ...) {

	RetransmissionContext** contexts = malloc(amount*sizeof(RetransmissionContext*));

	va_list args;
	va_start(args, amount);
	for(int i = 0; i < amount; i++) {
		contexts[i] = va_arg(args, RetransmissionContext*);
	}
	va_end(args);

	ComposeContextArgs* c_args = malloc(sizeof(ComposeContextArgs));
	c_args->contexts = contexts;
	c_args->amount = amount;

	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

	r_context->context_state.args = c_args;
    r_context->context_state.vars = NULL;

	r_context->init = &ComposeContextInit;
	r_context->create_header = &ComposeContextHeader;
	r_context->process_event = &ComposeContextEvent;
	r_context->query_handler = &ComposeContextQuery;
	r_context->query_header_handler = &ComposeContextQueryHeader;
    r_context->copy_handler = &ComposeContextCopy;
    r_context->destroy = &ComposeContextDestroy;

	return r_context;
}
