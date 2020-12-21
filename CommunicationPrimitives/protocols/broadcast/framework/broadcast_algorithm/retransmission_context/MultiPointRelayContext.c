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

#include "protocols/discovery/framework/framework.h"

#include <assert.h>

typedef struct _MPRContextState {
	list* broadcast_mprs;
    list* broadcast_mpr_selectors;
} MPRContextState;

typedef struct _MPRContextArgs {
	RetransmissionContext* neighbors_context;
} MPRContextArgs;


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

    RC_init(args->neighbors_context, protocol_definition, myID, visited);
}

static void MPRContextEvent(ModuleState* context_state, queue_t_elem* elem, unsigned char* myID, list* visited) {
    MPRContextState* state = (MPRContextState*)(context_state->vars);
    MPRContextArgs* args = (MPRContextArgs*)(context_state->args);

    // Deliver the event to the neighbors_context
    RC_processEvent(args->neighbors_context, elem, myID, visited);

    if( elem->type == YGG_EVENT ) {
        YggEvent* ev = &elem->data.event;
        if( ev->notification_id == GENERIC_DISCOVERY_EVENT ) {
            unsigned int str_len = 0;
            void* ptr = NULL;
            ptr = YggEvent_readPayload(ev, ptr, &str_len, sizeof(unsigned int));

            char type[str_len+1];
            ptr = YggEvent_readPayload(ev, ptr, type, str_len*sizeof(char));
            type[str_len] = '\0';

            if( strcmp(type, "MPRS") == 0 ) {
                list_delete(state->broadcast_mprs);
                state->broadcast_mprs = list_init();

                unsigned int amount = 0;
                ptr = YggEvent_readPayload(ev, ptr, &amount, sizeof(unsigned int));

                for(int i = 0; i < amount; i++) {
                    unsigned char* id = malloc(sizeof(uuid_t));
                    ptr = YggEvent_readPayload(ev, ptr, id, sizeof(uuid_t));

                    list_add_item_to_tail(state->broadcast_mprs, id);
                }

                // Ignore Routing MPRS
            } else if( strcmp(type, "MPR SELECTORS") == 0 ) {
                list_delete(state->broadcast_mpr_selectors);
                state->broadcast_mpr_selectors = list_init();

                unsigned int amount = 0;
                ptr = YggEvent_readPayload(ev, ptr, &amount, sizeof(unsigned int));

                for(int i = 0; i < amount; i++) {
                    unsigned char* id = malloc(sizeof(uuid_t));
                    ptr = YggEvent_readPayload(ev, ptr, id, sizeof(uuid_t));

                    list_add_item_to_tail(state->broadcast_mpr_selectors, id);
                }

                // Ignore Routing MPR SELECTORS
            }
        }
    }
}

static bool MPRContextQuery(ModuleState* context_state, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {

    MPRContextState* state = (MPRContextState*) (context_state->vars);
    MPRContextArgs* args = (MPRContextArgs*)(context_state->args);

	if( strcmp(query, "mprs") == 0 || strcmp(query, "broadcast_mprs") == 0 || strcmp(query, "delegated_neighbors") == 0 ) {
        *((list**)result) = list_clone(state->broadcast_mprs, sizeof(uuid_t));
		return true;
	} else if( strcmp(query, "mpr_selectors") == 0 || strcmp(query, "broadcast_mpr_selectors") == 0 ) {
        *((list**)result) = list_clone(state->broadcast_mpr_selectors, sizeof(uuid_t));
		return true;
	} else if( strcmp(query, "delegated") == 0 ) {
        assert(query_args);
        void** aux1 = hash_table_find_value(query_args, "p_msg");
        assert(aux1);
		PendingMessage* p_msg = *aux1;
        assert(p_msg);

        unsigned char* parent_id = getBcastHeader((((MessageCopy*)getCopies(p_msg)->head->data)))->sender_id;

        *((bool*)result) = (list_find_item(state->broadcast_mpr_selectors, &equalID, parent_id) != NULL);
		return true;
	}  else {
        return RC_query(args->neighbors_context, query, result, query_args, myID, visited);
    }
}

/*
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
*/

/*
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
*/

static void MultiPointRelayContextDestroy(ModuleState* context_state, list* visited) {
    MPRContextArgs* args = context_state->args;
    MPRContextState* state = context_state->vars;

    destroyRetransmissionContext(args->neighbors_context, visited);
    free(args);

    list_delete(state->broadcast_mprs);
    list_delete(state->broadcast_mpr_selectors);
    free(state);
}

RetransmissionContext* MultiPointRelayContext(RetransmissionContext* neighbors_context) {

	MPRContextArgs* args = malloc(sizeof(MPRContextArgs));
    args->neighbors_context = neighbors_context;

    MPRContextState* state = malloc(sizeof(MPRContextState));
    state->broadcast_mprs = list_init();
    state->broadcast_mpr_selectors = list_init();

    return newRetransmissionContext(
        args,
        state,
        &MPRContextInit,
        &MPRContextEvent,
        NULL,
        &MPRContextQuery,
        NULL,
        NULL,
        &MultiPointRelayContextDestroy
    );
}
