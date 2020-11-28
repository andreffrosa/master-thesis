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

typedef struct MonitorContextArgs_ {
     char window_type[100];
     unsigned long log_period;
     RetransmissionContext* child_context;
} MonitorContextArgs;

typedef struct MonitorContextState_ {
    uuid_t timer_id;
} MonitorContextState;


static void MonitorContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    MonitorContextArgs* args = (MonitorContextArgs*)(context_state->args);
    MonitorContextState* state = (MonitorContextState*)(context_state->vars);

    struct timespec t;
    milli_to_timespec(&t, args->log_period);
    SetPeriodicTimer(&t, state->timer_id, proto_def_getId(protocol_definition), 3);

    if(list_find_item(visited, &equalAddr, args->child_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->child_context;
        list_add_item_to_tail(visited, this);

        args->child_context->init(&args->child_context->context_state, protocol_definition, myID, visited);
    }
}

static unsigned int MonitorContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    MonitorContextArgs* args = (MonitorContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->child_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->child_context;
        list_add_item_to_tail(visited, this);

        return args->child_context->create_header(&args->child_context->context_state, p_msg, context_header, r_context, myID, visited);
    }

    *context_header = NULL;
    return 0;
}

static void MonitorContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    MonitorContextArgs* args = (MonitorContextArgs*)(context_state->args);
    MonitorContextState* state = (MonitorContextState*)(context_state->vars);

    if(elem->type == YGG_TIMER) {
		if(uuid_compare(elem->data.timer.id, state->timer_id) == 0 ) {
            // Log
            struct timespec current_time;
            clock_gettime(CLOCK_MONOTONIC, &current_time);

            double stability;
            if(!query_context(r_context, "stability", &stability, myID, 2, &current_time, args->window_type))
                assert(false);

            double density;
            if(!query_context(r_context, "avg_neighbors", &density, myID, 1, true))
                assert(false);

            double in_traffic;
            if(!query_context(r_context, "in_traffic", &in_traffic, myID, 2, &current_time, args->window_type))
                assert(false);

            double out_traffic;
            if(!query_context(r_context, "out_traffic", &out_traffic, myID, 2, &current_time, args->window_type))
                assert(false);

            double misses;
            if(!query_context(r_context, "misses", &misses, myID, 2, &current_time, args->window_type))
                assert(false);

            char str[500];
            sprintf(str, "stability: %f changes/s density: %f neighs in_traffic: %f msgs/s out_traffic: %f msgs/s misses %f", stability, density, in_traffic, out_traffic, misses);
            ygg_log("MONITOR CONTEXT", "LOG", str);
        }
    }

    if(list_find_item(visited, &equalAddr, args->child_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->child_context;
        list_add_item_to_tail(visited, this);

        args->child_context->process_event(&args->child_context->context_state, elem, r_context, myID, visited);
    }
}

static bool MonitorContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    MonitorContextArgs* args = (MonitorContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->child_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->child_context;
        list_add_item_to_tail(visited, this);

        return args->child_context->query_handler(&args->child_context->context_state, query, result, argc, argv, r_context, myID, visited);
    }

    return false;
}

static bool MonitorContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	MonitorContextArgs* args = (MonitorContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->child_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->child_context;
        list_add_item_to_tail(visited, this);

        return args->child_context->query_header_handler(&args->child_context->context_state, context_header, context_header_size, query, result, argc, argv, r_context, myID, visited);
    }

    return false;
}

static void MonitorContextCopy(ModuleState* context_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    MonitorContextArgs* args = (MonitorContextArgs*)(context_state->args);

    if(args->child_context->copy_handler != NULL) {
        if(list_find_item(visited, &equalAddr, args->child_context) == NULL) {
            void** this = malloc(sizeof(void*));
            *this = args->child_context;
            list_add_item_to_tail(visited, this);

            args->child_context->copy_handler(&args->child_context->context_state, p_msg, r_context, myID, visited);
        }
    }
}

static void MonitorContextDestroy(ModuleState* context_state, list* visited) {
    MonitorContextArgs* state = (MonitorContextArgs*)(context_state->vars);
    free(state);

    MonitorContextArgs* args = (MonitorContextArgs*)(context_state->args);
    if(list_find_item(visited, &equalAddr, args->child_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->child_context;
        list_add_item_to_tail(visited, this);

        destroyRetransmissionContext(args->child_context, visited);
    }
    free(args);
}

RetransmissionContext* MonitorContext(unsigned long log_period, char* window_type, RetransmissionContext* child_context) {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

    MonitorContextArgs* args = malloc(sizeof(MonitorContextArgs));
    args->log_period = log_period;
    args->child_context = child_context;
    strcpy(args->window_type, window_type);
	r_context->context_state.args = args;

    MonitorContextState* state = malloc(sizeof(MonitorContextState));
    uuid_generate_random(state->timer_id);
    r_context->context_state.vars = state;

	r_context->init = &MonitorContextInit;
	r_context->create_header = &MonitorContextHeader;
	r_context->process_event = &MonitorContextEvent;
	r_context->query_handler = &MonitorContextQuery;
	r_context->query_header_handler = &MonitorContextQueryHeader;
    r_context->copy_handler = &MonitorContextCopy;
    r_context->destroy = &MonitorContextDestroy;

	return r_context;
}
