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
#include "utility/my_string.h"

#include <assert.h>

#include "protocols/discovery/framework/framework.h"

typedef struct LabelNeighsContextState_ {
    hash_table* labels;
} LabelNeighsContextState;

typedef struct NeighLabels_ {
    NeighCoverageLabel in_label;
    NeighCoverageLabel out_label;
} NeighLabels;

static hash_table* compute_labels(graph* neighborhood, unsigned char* myID);

static NeighCoverageLabel compute_label(graph* neighborhood, graph_node* node_a, graph_node* node_b);

static double missedCriticalNeighs(PendingMessage* p_msg, graph* neighborhood, hash_table* labels, unsigned char* myID);

//static void append_labels(graph* neighborhood);

/*
static void LabelNeighsContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    LabelNeighsContextArgs* args = (LabelNeighsContextArgs*)(context_state->args);

    RC_init(args->neighbors_context, protocol_definition, myID, visited);
}
*/

static void LabelNeighsContextEvent(ModuleState* context_state, queue_t_elem* elem, unsigned char* myID, hash_table* contexts) {

	LabelNeighsContextState* state = (LabelNeighsContextState*)(context_state->vars);

	if(elem->type == YGG_EVENT) {
        YggEvent* ev = &elem->data.event;
        //if( ev->notification_id == NEIGHBOR_FOUND /*|| ev->notification_id == NEIGHBOR_UPDATE*/ || ev->notification_id == NEIGHBOR_LOST ) {
        if( ev->notification_id == NEIGHBORHOOD ) {

            RetransmissionContext* neighbors_context = hash_table_find_value(contexts, "NeighborsContext");
            assert(neighbors_context);

            // Recompute Labels
            graph* neighborhood = NULL;
            if(!RC_query(neighbors_context, "neighborhood", &neighborhood, NULL, myID, contexts))
                assert(false);

            hash_table_delete(state->labels);
            state->labels = compute_labels(neighborhood, myID);

            graph_delete(neighborhood);
        }
	}
}

static bool LabelNeighsContextQuery(ModuleState* context_state, const char* query, void* result, hash_table* query_args, unsigned char* myID, hash_table* contexts) {

	LabelNeighsContextState* state = (LabelNeighsContextState*)(context_state->vars);

	if(strcmp(query, "node_in_label") == 0) {
        assert(query_args);
		unsigned char* id = hash_table_find_value(query_args, "id");
        assert(id);

        NeighLabels* neigh_labels = hash_table_find_value(state->labels, id);

        *((NeighCoverageLabel*)result) = neigh_labels ? neigh_labels->in_label : UNKNOWN_NODE;

        return true;
	} else if(strcmp(query, "node_out_label") == 0) {
        assert(query_args);
        unsigned char* id = hash_table_find_value(query_args, "id");
        assert(id);

        NeighLabels* neigh_labels = hash_table_find_value(state->labels, id);

        *((NeighCoverageLabel*)result) = neigh_labels ? neigh_labels->out_label : UNKNOWN_NODE;

        return true;
	} else if(strcmp(query, "missed_critical_neighs") == 0) {
        assert(query_args);
        void** aux1 = hash_table_find_value(query_args, "p_msg");
        assert(aux1);
		PendingMessage* p_msg = *aux1;
        assert(p_msg);

        RetransmissionContext* neighbors_context = hash_table_find_value(contexts, "NeighborsContext");
        assert(neighbors_context);

        graph* neighborhood = NULL;
        if(!RC_query(neighbors_context, "neighborhood", &neighborhood, NULL, myID, contexts))
            assert(false);

		*((double*)result) = missedCriticalNeighs(p_msg, neighborhood, state->labels, myID);
		return true;
	}

    return false;
}

static void LabelNeighsContextDestroy(ModuleState* context_state) {
    LabelNeighsContextState* state = context_state->vars;
    hash_table_delete(state->labels);
    free(state);
}

RetransmissionContext* LabelNeighsContext(RetransmissionContext* neighbors_context) {

    LabelNeighsContextState* state = malloc(sizeof(LabelNeighsContextState));
    state->labels = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    RetransmissionContext* ctx = newRetransmissionContext(
        "LabelNeighsContext",
        NULL,
        state,
        NULL, //&LabelNeighsContextInit,
        &LabelNeighsContextEvent,
        NULL,
        NULL,
        &LabelNeighsContextQuery,
        NULL,
        &LabelNeighsContextDestroy,
        new_list(1, new_str("NeighborsContext"))
    );

    return ctx;
}

static hash_table* compute_labels(graph* neighborhood, unsigned char* myID) {

    hash_table* labels = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    graph_node* me = graph_find_node(neighborhood, myID);
    assert(me);

    list* neighs = graph_get_adjacencies_from_node(neighborhood, me, IN_ADJ);
    for(list_item* it = neighs->head; it; it = it->next) {
        unsigned char* neigh_id = (unsigned char*)it->data;

        graph_node* neigh = graph_find_node(neighborhood, neigh_id);
        assert(neigh);

        NeighLabels* neigh_labels = malloc(sizeof(NeighLabels));
        neigh_labels->out_label = compute_label(neighborhood, me, neigh);
        neigh_labels->in_label = compute_label(neighborhood, neigh, me);

        unsigned char* id = malloc(sizeof(uuid_t));
        uuid_copy(id, neigh_id);
        hash_table_insert(labels, id, neigh_labels);
    }
    list_delete(neighs);

    return labels;
}

static NeighCoverageLabel compute_label(graph* neighborhood, graph_node* start_node, graph_node* end_node) {
    list* covered_by_start_node = graph_get_adjacencies_from_node(neighborhood, start_node, OUT_ADJ); // nodes that consider start_node as their neighbor
    list* covered_by_end_node = graph_get_adjacencies_from_node(neighborhood, end_node, OUT_ADJ); // nodes that consider end_node as their neighbor

    //bool start_is_neighbor_of_end = false, end_is_neighbor_of_start = false;

    void* tmp = list_remove_item(covered_by_start_node, &equalID, end_node->key);
    if(tmp) {
        free(tmp);
        //start_is_neighbor_of_end = true;
    }

    tmp = list_remove_item(covered_by_end_node, &equalID, start_node->key);
    if(tmp) {
        free(tmp);
        //end_is_neighbor_of_start = true;
    }

    /*if(!start_is_neighbor_of_end) {
        return UNKNOWN_NODE;
    }*/

    list* diff = list_difference(covered_by_end_node, covered_by_start_node, &equalID, neighborhood->key_size);
    if(diff->size > 0) {
        return CRITICAL_NODE;
    } else {
        if(covered_by_start_node->size == covered_by_end_node->size) {
            return REDUNDANT_NODE;
        } else {
            assert(covered_by_end_node->size < covered_by_start_node->size);
            return COVERED_NODE;
        }
    }

    free(diff);
    free(covered_by_start_node);
    free(covered_by_end_node);
}

/*
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
*/

// returns the fraction of critical neighs missed over all critical neighs (only SYM are considered)
static double missedCriticalNeighs(PendingMessage* p_msg, graph* neighborhood, hash_table* labels, unsigned char* myID) {
	unsigned int missed = 0;
	unsigned int critical_neighs = 0;

    list* neighs = graph_get_adjacencies(neighborhood, myID, SYM_ADJ);
    for(list_item* it = neighs->head; it; it = it->next) {
        unsigned char* neigh_id = it->data;

        NeighLabels* neigh_labels = hash_table_find_value(labels, neigh_id);
        assert(neigh_labels);

        if( neigh_labels->out_label == CRITICAL_NODE ) {
            if( !receivedCopyFrom(p_msg, neigh_id) )
				missed++;

			critical_neighs++;
        }
    }
    list_delete(neighs);

    double result = critical_neighs == 0 ? 0.0 : ((double)missed) / critical_neighs;
    assert(0.0 <= result && result <= 1.0);
	return result;
}
