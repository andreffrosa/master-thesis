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
        RC_init(args->contexts[i], protocol_definition, myID, visited);
	}
}

static unsigned int ComposeContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, unsigned char* myID, list* visited) {
	ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));

	unsigned char sizes[args->amount];
    void* headers[args->amount];
    unsigned int total_size = sizeof(sizes);

    for(int i = 0; i < args->amount; i++) {
        sizes[i] = RC_createHeader(args->contexts[i], p_msg, &headers[i], myID, visited);

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

static void ComposeContextEvent(ModuleState* context_state, queue_t_elem* elem, unsigned char* myID, list* visited) {
	ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));

    for(int i = 0; i < args->amount; i++) {
        RC_processEvent(args->contexts[i], elem, myID, visited);
	}
}

static bool ComposeContextQuery(ModuleState* context_state, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {

	ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));
	for(int i = 0; i < args->amount; i++) {
        bool r = RC_query(args->contexts[i], query, result, query_args, myID, visited);
        if(r) return true;
	}

	return false;
}

static bool ComposeContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {

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
            bool r = RC_queryHeader(args->contexts[i], ptr, sizes[i], query, result, query_args, myID, visited);
            if(r) return true;
		}

        ptr += sizes[i];
	}
    assert(ptr == context_header + context_header_size);

	return false;
}

static void ComposeContextCopy(ModuleState* context_state, PendingMessage* p_msg, unsigned char* myID, list* visited) {
    ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));

    for(int i = 0; i < args->amount; i++) {
        RC_processCopy(args->contexts[i], p_msg, myID, visited);
    }
}

static void ComposeContextDestroy(ModuleState* context_state, list* visited) {
    ComposeContextArgs* args = ((ComposeContextArgs*)(context_state->args));

    for(int i = 0; i < args->amount; i++) {
        destroyRetransmissionContext(args->contexts[i], visited);
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

    return newRetransmissionContext(
        c_args,
        NULL,
        &ComposeContextInit,
        &ComposeContextEvent,
        &ComposeContextHeader,
        &ComposeContextQuery,
        &ComposeContextQueryHeader,
        &ComposeContextCopy,
        &ComposeContextDestroy
    );
}
