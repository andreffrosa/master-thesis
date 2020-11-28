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

#include "../common.h"

typedef struct _RetransmissionContext {
    ModuleState context_state;
	void (*init)(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited);
	void (*process_event)(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited);
	unsigned int (*create_header)(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited);
	bool (*query_handler)(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited);
	bool (*query_header_handler)(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited);
    void (*copy_handler)(ModuleState* context_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID, list* visited);
    void (*destroy)(ModuleState* context_state, list* visited);
} RetransmissionContext;

#endif /* _RETRANSMISSION_CONTEXT_PRIVATE_H_ */
