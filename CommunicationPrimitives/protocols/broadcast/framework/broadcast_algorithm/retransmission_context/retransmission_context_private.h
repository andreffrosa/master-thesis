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

#ifndef _RETRANSMISSION_CONTEXT_PRIVATE_H_
#define _RETRANSMISSION_CONTEXT_PRIVATE_H_

#include "retransmission_context.h"

typedef void (*init_ctx)(ModuleState* state, proto_def* protocol_definition, unsigned char* myID, list* visited);

typedef void (*ctx_event_handler)(ModuleState* state, queue_t_elem* elem, unsigned char* myID, list* visited);

typedef unsigned int (*create_ctx_header)(ModuleState* state, PendingMessage* p_msg, void** context_header, unsigned char* myID, list* visited);

typedef bool (*ctx_query_handler)(ModuleState* state, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited);

typedef bool (*ctx_query_header_handler)(ModuleState* state, void* context_header, unsigned int context_header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited);

typedef void (*ctx_copy_handler)(ModuleState* state, PendingMessage* p_msg, unsigned char* myID, list* visited);

typedef void (*destroy_ctx)(ModuleState* state, list* visited);

typedef struct _RetransmissionContext {
    ModuleState state;

	init_ctx init;
	ctx_event_handler process_event;
    create_ctx_header create_header;
    ctx_query_handler query_handler;
    ctx_query_header_handler query_header_handler;
    ctx_copy_handler copy_handler;
    destroy_ctx destroy;
} RetransmissionContext;

RetransmissionContext* newRetransmissionContext(void* args, void* vars, init_ctx init, ctx_event_handler process_event, create_ctx_header create_header, ctx_query_handler query_handler, ctx_query_header_handler query_header_handler, ctx_copy_handler copy_handler, destroy_ctx destroy);

#endif /* _RETRANSMISSION_CONTEXT_PRIVATE_H_ */
