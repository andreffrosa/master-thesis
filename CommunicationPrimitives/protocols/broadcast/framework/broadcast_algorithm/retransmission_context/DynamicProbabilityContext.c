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

typedef struct _DynamicProbabilityContextArgs {
    double p_l;
    double p_u;
    double d;
    unsigned long t;
} DynamicProbabilityContextArgs;

typedef struct _DynamicProbabilityContextState {
    double p;
    unsigned int counter;
    uuid_t timer_id;
} DynamicProbabilityContextState;

static void DynamicProbabilityContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    DynamicProbabilityContextArgs* args = (DynamicProbabilityContextArgs*)(context_state->args);
    DynamicProbabilityContextState* state = (DynamicProbabilityContextState*)(context_state->vars);

    struct timespec t;
    milli_to_timespec(&t, args->t);
    SetPeriodicTimer(&t, state->timer_id, proto_def_getId(protocol_definition), 3);
}

static unsigned int DynamicProbabilityContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	*context_header = NULL;
	return 0;
}

static void DynamicProbabilityContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    DynamicProbabilityContextArgs* args = (DynamicProbabilityContextArgs*)(context_state->args);
    DynamicProbabilityContextState* state = (DynamicProbabilityContextState*)(context_state->vars);

    if(elem->type == YGG_TIMER) {
		if(uuid_compare(elem->data.timer.id, state->timer_id) == 0 ) {
            if(state->counter == 0) {
                state->p = state->p*2.0;
                if(state->p > args->p_u)
                    state->p = args->p_u;
            }
            state->counter = 0;
        }
    }
}

static bool DynamicProbabilityContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

    DynamicProbabilityContextState* state = (DynamicProbabilityContextState*)(context_state->vars);

	if(strcmp(query, "p") == 0 || strcmp(query, "probability") == 0) {
		*((double*)result) = state->p;
        return true;
	}

	return false;
}

static bool DynamicProbabilityContextQueryHeader(ModuleState* context_state, void* header, unsigned int header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	return false;
}

static void DynamicProbabilityContextCopy(ModuleState* context_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    DynamicProbabilityContextArgs* args = (DynamicProbabilityContextArgs*)(context_state->args);
    DynamicProbabilityContextState* state = (DynamicProbabilityContextState*)(context_state->vars);

    if(/*isPendingMessageActive(p_msg) && getCurrentPhase(p_msg) == 1 &&*/ getCopies(p_msg)->size > 1) {
        state->p = state->p - args->d;
        if(state->p < args->p_l)
            state->p = args->p_l;
    }
    state->counter++;
}

static void DynamicProbabilityContextDestroy(ModuleState* context_state, list* visited) {
    free(context_state->args);
    free(context_state->vars);
}

RetransmissionContext* DynamicProbabilityContext(double p, double p_l, double p_u, double d, unsigned long t) {
    assert(0.0 <= p_l && p_l <= p && p <= p_u && p_u <= 1.0);
    assert(0.0 <= d && d <= 1.0 && d <= (p_u - p_l));
    assert(t > 0);

	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

    DynamicProbabilityContextArgs* args = malloc(sizeof(DynamicProbabilityContextArgs));
    args->p_l = p_l;
    args->p_u = p_u;
    args->d = d;
    args->t = t;
	r_context->context_state.args = args;

    DynamicProbabilityContextState* state = malloc(sizeof(DynamicProbabilityContextState));
    state->p = p;
    state->counter = 0;
    uuid_generate_random(state->timer_id);
    r_context->context_state.vars = state;

	r_context->init = &DynamicProbabilityContextInit;
	r_context->create_header = &DynamicProbabilityContextHeader;
	r_context->process_event = &DynamicProbabilityContextEvent;
	r_context->query_handler = &DynamicProbabilityContextQuery;
	r_context->query_header_handler = &DynamicProbabilityContextQueryHeader;
    r_context->copy_handler = &DynamicProbabilityContextCopy;
    r_context->destroy = &DynamicProbabilityContextDestroy;

	return r_context;
}
