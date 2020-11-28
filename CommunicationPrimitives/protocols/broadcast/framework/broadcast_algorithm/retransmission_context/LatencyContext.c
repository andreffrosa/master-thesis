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

#include "utility/my_time.h"

#include <assert.h>

static void LatencyContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
	// Do nothing
}

static unsigned int LatencyContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

    unsigned long* t = malloc(sizeof(unsigned long));

    assert(getCopies(p_msg)->size > 0);

    //if (getCopies(p_msg)->size > 0) {
        struct timespec* reception_time = getCopyReceptionTime(((message_copy*)getCopies(p_msg)->head->data));
        struct timespec current_time = {0}, elapsed = {0};
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        subtract_timespec(&elapsed, &current_time, reception_time);

        *t = timespec_to_milli(&elapsed);
    /*} else {
        *t = 0L;
    }*/

	*context_header = t;
	return sizeof(unsigned long);
}

static void LatencyContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	// Do nothing
}

static bool LatencyContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	return false;
}

static bool LatencyContextQueryHeader(ModuleState* context_state, void* header, unsigned int header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

    if(strcmp(query, "latency") == 0) {
        if(header) {
            *((unsigned long*)result) = *((unsigned long*)header);
        } else {
            *((unsigned long*)result) = 0L;
        }

		return true;
	}

	return false;
}

static void LatencyContextCopy(ModuleState* context_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

    if(getCurrentPhase(p_msg) == 1 && getCopies(p_msg)->size == 1) {
        message_copy* first = ((message_copy*)getCopies(p_msg)->head->data);
        unsigned long latency = 0L;
        if(!query_context_header(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "latency", &latency, myID, 0))
    		assert(false);

        char str[100];
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(getPendingMessageID(p_msg), id_str);
        sprintf(str, "[%s] latency = %lu", id_str, latency);
        ygg_log("LATENCY CONTEXT", "LATENCY", str);
    }
}

RetransmissionContext* LatencyContext() {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

	r_context->context_state.args = NULL;
    r_context->context_state.vars = NULL;

	r_context->init = &LatencyContextInit;
	r_context->create_header = &LatencyContextHeader;
	r_context->process_event = &LatencyContextEvent;
	r_context->query_handler = &LatencyContextQuery;
	r_context->query_header_handler = &LatencyContextQueryHeader;
    r_context->copy_handler = &LatencyContextCopy;
    r_context->destroy = NULL;

	return r_context;
}
