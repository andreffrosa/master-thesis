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

typedef struct _MPRContextState {
    graph* neighborhood;
	list* mprs;
} MPRContextState;

typedef struct _MPRContextArgs {
	RetransmissionContext* neighbors_context;
    double hyst_threshold_low;
    double hyst_threshold_high;
} MPRContextArgs;

/*
typedef struct _edge_label {
    double quality;
    bool pending;
} edge_label;
*/



// select only th bidirectional neighbors
// Stability of the links is being ignored
/*static list* get_bidirectional_neighbors(graph* neighborhood, unsigned char* id) {
    list* neighbors = list_init();

    graph_node* node = graph_find_node(neighborhood, id);
    for(list_item* it = node->in_adjacencies->head; it; it = it->next) {
        graph_edge* edge = it->data;

        // If the inverse edge exists
        if(graph_find_edge(neighborhood, edge->end_node->key, edge->start_node->key) != NULL) {
            unsigned char* neigh_id = malloc(sizeof(uuid_t));
            uuid_copy(neigh_id, edge->end_node->key);
            list_add_item_to_tail(neighbors, neigh_id);
        }
    }

    return neighbors;
}*/

static void MPRContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    MPRContextArgs* args = (MPRContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

	    args->neighbors_context->init(&args->neighbors_context->context_state, protocol_definition, myID, visited);
    }
}

/*static hash_table* update_pending(graph* neighborhood, unsigned char* id, hash_table* old_pending_neighbors, double hyst_threshold_low, double hyst_threshold_high) {
    hash_table* new_pending_neighbors = hash_table_init((hashing_function) &uuid_hash, (comparator_function) &equalID);

    graph_node* node = graph_find_node(neighborhood, id);
    for(list_item* it = node->in_adjacencies->head; it; it = it->next) {
        graph_edge* edge = it->data;

        bool* old_pending_ptr = (bool*) hash_table_find_value(old_pending_neighbors, edge->start_node->key);
        bool old_pending = false;
        if(old_pending_ptr == NULL) {
            old_pending = false; // initial value
        } else {
            old_pending = *old_pending_ptr;
        }

        bool* new_pending = malloc(sizeof(bool));
        double link_quality = *((double*)edge->label);

        if(link_quality > hyst_threshold_high) {
            *new_pending = false;
        }
        else if(link_quality < hyst_threshold_low) {
            *new_pending = false;
        } else {
            *new_pending = old_pending;
        }

        unsigned char* neigh_id = malloc(sizeof(uuid_t));
        uuid_copy(neigh_id, edge->start_node->key);
        hash_table_insert(new_pending_neighbors, neigh_id, new_pending);
    }

    hash_table_delete(old_pending_neighbors);

    return new_pending_neighbors;
}*/

/*static*/ void update_pending(graph* new_neighborhood, graph* old_neighborhood, double hyst_threshold_low, double hyst_threshold_high) {

    for(list_item* it = new_neighborhood->edges->head; it; it = it->next) {
        graph_edge* edge = it->data;

        bool old_pending = false;
        graph_edge* old_edge = graph_find_edge(old_neighborhood, edge->start_node->key, edge->end_node->key);
        if(old_edge != NULL) {
            edge_label* ex = (edge_label*)old_edge->label;
            old_pending = ex->pending;
        }

        double link_quality = *((double*)(edge->label));

        bool new_pending = link_quality > hyst_threshold_high ? false : (link_quality < hyst_threshold_low ? true : old_pending);

        edge_label* ex2 = malloc(sizeof(edge_label));
        ex2->quality = link_quality;
        ex2->pending = new_pending;
        free(edge->label);
        edge->label = ex2;
    }

    new_neighborhood->label_size = sizeof(edge_label);
}

static void MPRContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

    MPRContextState* state = (MPRContextState*)(context_state->vars);
    MPRContextArgs* args = (MPRContextArgs*)(context_state->args);

    // Deliver the event to the neighbors_context
    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

        args->neighbors_context->process_event(&args->neighbors_context->context_state, elem, r_context, myID, visited);
    }

    // Update
    if(elem->type == YGG_EVENT) {
        if(elem->data.event.notification_id == NEIGHBORHOOD_UPDATE) {

                graph* new_neighborhood;
            	if(!query_context(args->neighbors_context, "neighborhood", &new_neighborhood, myID, 0))
            		assert(false);

                graph* old_neighborhood = state->neighborhood;

                update_pending(new_neighborhood, old_neighborhood, args->hyst_threshold_low, args->hyst_threshold_high);

                state->neighborhood = new_neighborhood;

                if(state->mprs != NULL)
                    list_delete(state->mprs);
                state->mprs = compute_mprs(new_neighborhood, myID);

                graph_delete(old_neighborhood);
        }
    }
}

static bool MPRContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

    MPRContextState* state = (MPRContextState*) (context_state->vars);
    MPRContextArgs* args = (MPRContextArgs*)(context_state->args);

	if( strcmp(query, "mprs") == 0 || strcmp(query, "delegated_neighbors") == 0 ) {
        *((list**)result) = list_clone(state->mprs, sizeof(uuid_t));
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

static bool MPRContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

	if( strcmp(query, "mprs") == 0 || strcmp(query, "delegated_neighbors") == 0 ) {
        list* l = list_init();

        int size = context_header_size / sizeof(uuid_t);
        for(int i = 0; i < size; i++) {
            unsigned char* id = (unsigned char*)(context_header + i*sizeof(uuid_t));
            unsigned char* id_copy = malloc(sizeof(uuid_t));
            uuid_copy(id_copy, id);
            list_add_item_to_tail(l, id_copy);
        }

		*((list**)result) = l;

		return true;
	}

	return false;
}

static unsigned int MPRContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    MPRContextState* state = (MPRContextState*) (context_state->vars);

    int size = state->mprs->size * sizeof(uuid_t);

    unsigned char* mprs = malloc(size);
    unsigned char* ptr = mprs;

    for(list_item* it = state->mprs->head; it; it = it->next) {
        uuid_copy(ptr, it->data);
        ptr += sizeof(uuid_t);
    }
    assert(ptr == mprs + size);

    *context_header = mprs;
	return size;
}

static void MultiPointRelayContextDestroy(ModuleState* context_state, list* visited) {
    MPRContextArgs* args = context_state->args;
    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

        destroyRetransmissionContext(args->neighbors_context, visited);
    }
    free(args);

    MPRContextState* state = context_state->vars;
    graph_delete(state->neighborhood);
    list_delete(state->mprs);
    free(state);
}

RetransmissionContext* MultiPointRelayContext(RetransmissionContext* neighbors_context, double hyst_threshold_low, double hyst_threshold_high) {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

	MPRContextArgs* args = malloc(sizeof(MPRContextArgs));
    args->neighbors_context = neighbors_context;
    args->hyst_threshold_low = hyst_threshold_low;
    args->hyst_threshold_high = hyst_threshold_high;
    r_context->context_state.args = args;

    MPRContextState* state = malloc(sizeof(MPRContextState));

    state->neighborhood = graph_init_complete((key_comparator)&uuid_compare, NULL, NULL, sizeof(uuid_t), 0, sizeof(double));

    state->mprs = list_init();
    r_context->context_state.vars = state;

	r_context->init = &MPRContextInit;
	r_context->create_header = &MPRContextHeader;
	r_context->process_event = &MPRContextEvent;
	r_context->query_handler = &MPRContextQuery;
	r_context->query_header_handler = &MPRContextQueryHeader;
    r_context->copy_handler = NULL;
    r_context->destroy = &MultiPointRelayContextDestroy;

	return r_context;
}
