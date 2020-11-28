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

#include <assert.h>

#include "data_structures/graph.h"

typedef struct _AHBPContextState {
    graph* neighborhood;
} AHBPContextState;

typedef struct _AHBPContextArgs {
	RetransmissionContext* neighbors_context;
    RetransmissionContext* route_context;
    double hyst_threshold_low;
    double hyst_threshold_high;
} AHBPContextArgs;

list* compute_mprs(graph* neighborhood, unsigned char* myID); // Defined in MultipointRelayContext.c
void update_pending(graph* new_neighborhood, graph* old_neighborhood, double hyst_threshold_low, double hyst_threshold_high);  // Defined in MultipointRelayContext.c

/*static */list* compute_bgrs(graph* neighborhood, unsigned char* myID, list* route) {
    graph* aux = graph_clone(neighborhood);

    char neigh_str[UUID_STR_LEN+1];
    neigh_str[UUID_STR_LEN] = '\0';

    for(list_item* it = route->head; it; it = it->next) {
        list* neighs = graph_get_adjacencies(aux, it->data, SYM_ADJ);

        if(neighs != NULL) {
            uuid_unparse(it->data, neigh_str);
            printf("   neigh: %s\n", neigh_str);

            for(list_item* it2 = neighs->head; it2; it2 = it2->next) {
                uuid_unparse(it2->data, neigh_str);
                printf("      neigh2: %s\n", neigh_str);

                if( uuid_compare(myID, it2->data) != 0 ) {
                    void* x = graph_remove_node(aux, it2->data);
                    if(x) free(x);
                    printf("      removed: %s\n", x ? "true" : "false");
                }
            }

            if( uuid_compare(myID, it->data) != 0 ) {
                void* x = graph_remove_node(aux, it->data);
                if(x) free(x);
                printf("   removed: %s\n", x ? "true" : "false");
            }
        } else {
            uuid_unparse(it->data, neigh_str);
            printf("   node %s not know\n", neigh_str);
        }
    }

    printf("MPRS:\n");

    list* bgrs = compute_mprs(aux, myID);

    graph_delete(aux);

    printf("Route:\n");
    for(list_item* it = route->head; it; it = it->next) {
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(((unsigned char*)it->data), id_str);
        printf("%s\n", id_str);
    }

    printf("Computed Delegated Neighbors:\n");
    for(list_item* it = bgrs->head; it; it = it->next) {
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(((unsigned char*)it->data), id_str);
        printf("%s\n", id_str);
    }

    return bgrs;
}

static void AHBPContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

        args->neighbors_context->init(&args->neighbors_context->context_state, protocol_definition, myID, visited);
    }

    if(list_find_item(visited, &equalAddr, args->route_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->route_context;
        list_add_item_to_tail(visited, this);

        args->route_context->init(&args->route_context->context_state, protocol_definition, myID, visited);
    }
}

static unsigned int AHBPContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);
    AHBPContextState* state = (AHBPContextState*)(context_state->vars);

    // Insert route
    void* route_header = NULL;
    unsigned int route_header_size = 0;
    if(list_find_item(visited, &equalAddr, args->route_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->route_context;
        list_add_item_to_tail(visited, this);

        route_header_size = args->route_context->create_header(&args->route_context->context_state, p_msg, &route_header, r_context, myID, visited);
    }

    // Insert BGRs
    list* route = NULL;
    double_list* copies = getCopies(p_msg);

    assert(copies->size > 0);

    //if(copies->size > 0) {
        message_copy* first = (message_copy*)(copies->head->data);
        if(!query_context_header(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "route", &route, myID, 0))
    		assert(false);
    /*} else {
        route = list_init();
    }*/

    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(getBcastHeader(first)->sender_id, id_str);
    printf("parent: %s\n", id_str);

    printf("neighborhood size: %d\n", state->neighborhood->nodes->size);

    list* bgrs = compute_bgrs(state->neighborhood, myID, route);

    int bgrs_header_size = bgrs->size * sizeof(uuid_t);
    unsigned char* bgrs_header = malloc(bgrs_header_size);
    unsigned char* ptr1 = bgrs_header;

    for(list_item* it = bgrs->head; it; it = it->next) {
        uuid_copy(ptr1, it->data);
        ptr1 += sizeof(uuid_t);
    }

    assert(route_header_size <= 255 && bgrs_header_size <= 255);

    int total_size = 2*sizeof(char) + route_header_size + bgrs_header_size;

    //assert(total_size <= 255);

    char aux = '\0';
    unsigned char* buffer = malloc(total_size);
    unsigned char* ptr = buffer;

    aux = route_header_size;
    memcpy(ptr, &aux, sizeof(char));
    ptr += sizeof(char);

    aux = bgrs_header_size;
    memcpy(ptr, &aux, sizeof(char));
    ptr += sizeof(char);

    memcpy(ptr, route_header, route_header_size);
    ptr += route_header_size;
    memcpy(ptr, bgrs_header, bgrs_header_size);

    free(route_header);
    free(bgrs_header);
    free(route);
    free(bgrs);

    *context_header = buffer;
	return total_size;
}

static void AHBPContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);
    AHBPContextState* state = (AHBPContextState*)(context_state->vars);

    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

	    args->neighbors_context->process_event(&args->neighbors_context->context_state, elem, r_context, myID, visited);
    }

    if(list_find_item(visited, &equalAddr, args->route_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->route_context;
        list_add_item_to_tail(visited, this);

        args->route_context->process_event(&args->route_context->context_state, elem, r_context, myID, visited);
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
                graph_delete(old_neighborhood);
        }
    }
}

static bool AHBPContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);

    if(list_find_item(visited, &equalAddr, args->route_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->route_context;
        list_add_item_to_tail(visited, this);

        bool r = args->route_context->query_handler(&args->route_context->context_state, query, result, argc, argv, r_context, myID, visited);
        if(r) {
            return r;
        }
    }

    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

        return args->neighbors_context->query_handler(&args->neighbors_context->context_state, query, result, argc, argv, r_context, myID, visited);
    }

    return false;
}

static bool AHBPContextQueryHeader(ModuleState* context_state, void* header, unsigned int header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {

    AHBPContextArgs* args = (AHBPContextArgs*)(context_state->args);

    char sizes[2];
    unsigned char* route_header = NULL;
    unsigned char* bgrs_header = NULL;

    if(header != NULL) {
        memcpy(sizes, header, sizeof(sizes));

        route_header = header + sizeof(sizes);
        bgrs_header = route_header + sizes[0];
    } else {
        memset(sizes, 0, sizeof(sizes));

        route_header = NULL;
        bgrs_header = NULL;
    }

    if( strcmp(query, "bgrs") == 0 || strcmp(query, "delegated_neighbors") == 0 ) {
        list* l = list_init();

        int size = sizes[1] / sizeof(uuid_t);
        for(int i = 0; i < size; i++) {
            unsigned char* id = bgrs_header + i*sizeof(uuid_t);
            unsigned char* id_copy = malloc(sizeof(uuid_t));
            uuid_copy(id_copy, id);
            list_add_item_to_tail(l, id_copy);
        }

		*((list**)result) = l;

		return true;
	} else {
        if(list_find_item(visited, &equalAddr, args->route_context) == NULL) {
            void** this = malloc(sizeof(void*));
            *this = args->route_context;
            list_add_item_to_tail(visited, this);

            return args->route_context->query_header_handler(&args->route_context->context_state, route_header, sizes[0], query, result, argc, argv, r_context, myID, visited);
        }
    }

    return false;
}

static void AHBPContextDestroy(ModuleState* context_state, list* visited) {
    AHBPContextArgs* args = context_state->args;

    if(list_find_item(visited, &equalAddr, args->neighbors_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->neighbors_context;
        list_add_item_to_tail(visited, this);

        destroyRetransmissionContext(args->neighbors_context, visited);
    }

    if(list_find_item(visited, &equalAddr, args->route_context) == NULL) {
        void** this = malloc(sizeof(void*));
        *this = args->route_context;
        list_add_item_to_tail(visited, this);

        destroyRetransmissionContext(args->route_context, visited);
    }

    free(args);

    AHBPContextState* state = context_state->vars;
    graph_delete(state->neighborhood);
    free(state);
}

RetransmissionContext* AHBPContext(RetransmissionContext* neighbors_context, RetransmissionContext* route_context, double hyst_threshold_low, double hyst_threshold_high) {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

    AHBPContextArgs* args = malloc(sizeof(AHBPContextArgs));
    args->neighbors_context = neighbors_context;
    args->route_context = route_context;
    args->hyst_threshold_low = hyst_threshold_low;
    args->hyst_threshold_high = hyst_threshold_high;
    r_context->context_state.args = args;

    AHBPContextState* state = malloc(sizeof(AHBPContextState));

    state->neighborhood = graph_init_complete((key_comparator)&uuid_compare, NULL, NULL, sizeof(uuid_t), 0, sizeof(double));

    r_context->context_state.vars = state;

	r_context->init = &AHBPContextInit;
	r_context->create_header = &AHBPContextHeader;
	r_context->process_event = &AHBPContextEvent;
	r_context->query_handler = &AHBPContextQuery;
	r_context->query_header_handler = &AHBPContextQueryHeader;
    r_context->copy_handler = NULL;
    r_context->destroy = &AHBPContextDestroy;

	return r_context;
}
