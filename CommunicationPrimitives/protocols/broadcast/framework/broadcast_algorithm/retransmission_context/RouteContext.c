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

#include "utility/my_math.h"

static bool RouteContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {

	if(strcmp(query, "route") == 0 || strcmp(query, "path") == 0) {
        unsigned int amount = context_header_size / sizeof(uuid_t);

		list* l = list_init();

		for(int i = 0; i < amount; i++) {
			unsigned char* id = malloc(sizeof(uuid_t));
			unsigned char* ptr = context_header + i*sizeof(uuid_t);
			uuid_copy(id, ptr);
			list_add_item_to_tail(l, id);
		}

        *((list**)result) = l;
		return true;
	}

	return false;
}

static unsigned int RouteContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, unsigned char* myID, list* visited) {
    double_list* copies = getCopies(p_msg);
    assert(copies->size > 0);
    /*if(copies->size == 0) {
        *context_header = NULL;
        return 0;
    }*/

    MessageCopy* first = (MessageCopy*)copies->head->data;

    list* route = NULL;
    if(!RouteContextQueryHeader(context_state, getContextHeader(first), getBcastHeader(first)->context_length, "route", &route, NULL, myID, NULL))
		assert(false);

    unsigned int max_len = *((unsigned int*)(context_state->args));
    //unsigned int real_len = iMin(max_len - 1, route->size);

    while (route->size > max_len - 1) {
        free(list_remove_head(route));
    }

    unsigned char* x = malloc(sizeof(uuid_t));
    uuid_copy(x, myID);
    list_add_item_to_tail(route, x);

    assert(route->size <= max_len);

    unsigned int size = route->size*sizeof(uuid_t);

    unsigned char* buffer = malloc(size);
    unsigned char* ptr = buffer;
    for(list_item* it = route->head; it; it = it->next) {
        unsigned char* id = (unsigned char*)(it->data);
        memcpy(ptr, id, sizeof(uuid_t));
        ptr += sizeof(uuid_t);
    }

    *context_header = buffer;
    return size;
}

static void RouteContextDestroy(ModuleState* context_state, list* visited) {
    free(context_state->args);
}

RetransmissionContext* RouteContext(unsigned int max_len) {
    assert(max_len > 0);

	unsigned int* max_len_arg = malloc(sizeof(unsigned int));
	*max_len_arg = max_len;

    return newRetransmissionContext(
        max_len_arg,
        NULL,
        NULL,
        NULL,
        &RouteContextHeader,
        NULL,
        &RouteContextQueryHeader,
        NULL,
        &RouteContextDestroy
    );
}
