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

#include "data_structures/graph.h"
#include "data_structures/hash_table.h"
#include "utility/my_misc.h"

#include <assert.h>

typedef struct _LabelNeighsContextState {
    graph* neighborhood;
} LabelNeighsContextState;

typedef struct _LabelNeighsContextArgs {
    RetransmissionContext* neighbors_context;
} LabelNeighsContextArgs;

typedef struct _edge_label_2 {
    double quality;
    LabelNeighs_NodeLabel label;
} edge_label_2;

static double missedCriticalNeighs(PendingMessage* p_msg, graph* neighborhood, unsigned char* myID);

static void LabelNeighsContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    LabelNeighsContextArgs* args = (LabelNeighsContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

        args->neighbors_context->init(&args->neighbors_context->context_state, protocol_definition, myID, visited);
    }
}

static unsigned int LabelNeighsContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    LabelNeighsContextArgs* args = (LabelNeighsContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

	    return args->neighbors_context->create_header(&args->neighbors_context->context_state, p_msg, context_header, r_context, myID, visited);
    }

    *context_header = NULL;
    return 0;
}

/*static*/ LabelNeighs_NodeLabel compute_label(graph* neighborhood, graph_node* node_a, graph_node* node_b) {
    list* neighs_a = graph_get_adjacencies_from_node(neighborhood, node_a, OUT_ADJ);
    list* neighs_b = graph_get_adjacencies_from_node(neighborhood, node_b, OUT_ADJ);

    //bool a_in_b = false;
    bool b_in_a = false;

    void* tmp = list_remove_item(neighs_a, &equalID, node_b->key);
    if(tmp) {
        free(tmp);
        b_in_a = true;
    }

    tmp = list_remove_item(neighs_b, &equalID, node_a->key);
    if(tmp) {
        free(tmp);
        //a_in_b = true;
    }

    if(!b_in_a) {
        return UNKNOWN_NODE;
    }

    list* diff = list_difference(neighs_b, neighs_a, &equalID, neighborhood->key_size);
    if(diff->size > 0) {
        return CRITICAL_NODE;
    } else {
        if(neighs_a->size == neighs_b->size) {
            return REDUNDANT_NODE;
        } else {
            assert(neighs_b->size < neighs_a->size);
            return COVERED_NODE;
        }
    }

    free(diff);
    free(neighs_a);
    free(neighs_b);
}

static void append_labels(graph* neighborhood) {

    for(list_item* it = neighborhood->edges->head; it; it = it->next) {
        graph_edge* edge = it->data;

        edge_label_2* ex = malloc(sizeof(edge_label_2));
        ex->quality = *((double*)edge->label);

        ex->label = compute_label(neighborhood, edge->start_node, edge->end_node);

        free(edge->label);
        edge->label = ex;
    }

    neighborhood->label_size = sizeof(edge_label_2);
}

static void LabelNeighsContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

	LabelNeighsContextState* state = (LabelNeighsContextState*)(context_state->vars);
    LabelNeighsContextArgs* args = (LabelNeighsContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

	    args->neighbors_context->process_event(&args->neighbors_context->context_state, elem, r_context, myID, visited);
    }

	if(elem->type == YGG_EVENT) {
		if(elem->data.event.notification_id == NEIGHBORHOOD_UPDATE) {

            graph* new_neighborhood = NULL;
            if(!query_context(args->neighbors_context, "neighborhood", &new_neighborhood, myID, 0))
                assert(false);

            append_labels(new_neighborhood);

            graph_delete(state->neighborhood);
            state->neighborhood = new_neighborhood;
        }
	}
}

static bool LabelNeighsContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

	LabelNeighsContextState* state = (LabelNeighsContextState*)(context_state->vars);
    LabelNeighsContextArgs* args = (LabelNeighsContextArgs*)(context_state->args);

	if(strcmp(query, "node_in_label") == 0) {
		assert(argc >= 1);
		unsigned char* id = va_arg(*argv, unsigned char*);

        edge_label_2* label = (edge_label_2*)graph_find_label(state->neighborhood, id, myID);
        if(label) {
            *((LabelNeighs_NodeLabel*)result) = label->label;
		} else {
            *((LabelNeighs_NodeLabel*)result) = UNKNOWN_NODE;
        }
        return true;
	} else if(strcmp(query, "node_out_label") == 0) {
		assert(argc >= 1);
		unsigned char* id = va_arg(*argv, unsigned char*);

        edge_label_2* label = (edge_label_2*)graph_find_label(state->neighborhood, myID, id);
        if(label) {
            *((LabelNeighs_NodeLabel*)result) = label->label;
		} else {
            *((LabelNeighs_NodeLabel*)result) = UNKNOWN_NODE;
        }
        return true;
	} else if(strcmp(query, "missed_critical_neighs") == 0) {
		assert(argc >= 1);
		PendingMessage* p_msg = va_arg(*argv, PendingMessage*);

		*((double*)result) = missedCriticalNeighs(p_msg, state->neighborhood, myID);
		return true;
	} else {
        if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
            void** this = malloc(sizeof(void*));
            *this = args->neighbors_context;
            list_add_item_to_tail(visited, this);

		    return args->neighbors_context->query_handler(&args->neighbors_context->context_state, query, result, argc, argv, r_context, myID, visited);
        } else {
            return false;
        }
    }
}

static bool LabelNeighsContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    LabelNeighsContextArgs* args = (LabelNeighsContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

	    return args->neighbors_context->query_header_handler(&args->neighbors_context->context_state, context_header, context_header_size, query, result, argc, argv, r_context, myID, visited);
    }

    return false;
}

// returns the fraction of critical neighs missed over all critical neighs
static double missedCriticalNeighs(PendingMessage* p_msg, graph* neighborhood, unsigned char* myID) {
	unsigned int missed = 0;
	unsigned int critical_neighs = 0;

    list* neighs = graph_get_adjacencies(neighborhood, myID, SYM_ADJ);
    for(list_item* it = neighs->head; it; it = it->next) {
        unsigned char* neigh_id = it->data;
        graph_edge* edge = graph_find_edge(neighborhood, myID, neigh_id);
        assert(edge!=NULL);
        LabelNeighs_NodeLabel outLabel = ((edge_label_2*)edge->label)->label;
        if( outLabel == CRITICAL_NODE ) {
            if( !receivedCopyFrom(p_msg, neigh_id) )
				missed++;

			critical_neighs++;
        }
    }
    free(neighs);

    double result = critical_neighs == 0 ? 0.0 : ((double)missed) / critical_neighs;
    assert(0.0 <= result && result <= 1.0);
	return result;
}

static void LabelNeighsContextDestroy(ModuleState* context_state, list* visited) {
    LabelNeighsContextState* state = context_state->vars;
    graph_delete(state->neighborhood);
    free(state);

    LabelNeighsContextArgs* args = context_state->args;
    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

        destroyRetransmissionContext(args->neighbors_context, visited);
    }

    free(args);
}

RetransmissionContext* LabelNeighsContext(RetransmissionContext* neighbors_context) {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

    LabelNeighsContextState* state = malloc(sizeof(LabelNeighsContextState));
    state->neighborhood = NULL;

    LabelNeighsContextArgs* args = malloc(sizeof(LabelNeighsContextArgs));
    args->neighbors_context = neighbors_context;

	r_context->context_state.args = args;
    r_context->context_state.vars = state;

	r_context->init = &LabelNeighsContextInit;
	r_context->create_header = &LabelNeighsContextHeader;
	r_context->process_event = &LabelNeighsContextEvent;
	r_context->query_handler = &LabelNeighsContextQuery;
	r_context->query_header_handler = &LabelNeighsContextQueryHeader;
    r_context->copy_handler = NULL;
    r_context->destroy = &LabelNeighsContextDestroy;

	return r_context;
}
