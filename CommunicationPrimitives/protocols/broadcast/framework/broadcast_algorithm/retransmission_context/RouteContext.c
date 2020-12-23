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

static list* getRoute(double_list* copies, unsigned char* myID, unsigned int max_len) {
    assert(copies->size > 0);

    MessageCopy* first = (MessageCopy*)copies->head->data;
    hash_table* headers = getHeaders(first);

    list* route = (list*)hash_table_find_value(headers, "route");
    if(route == NULL) {
        route = list_init();
    }

    while (route->size > max_len - 1) {
        free(list_remove_head(route));
    }

    unsigned char* x = malloc(sizeof(uuid_t));
    uuid_copy(x, myID);
    list_add_item_to_tail(route, x);

    assert(route->size <= max_len);

    return route;
}

static void RouteContextAppendHeaders(ModuleState* context_state, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time) {

    double_list* copies = getCopies(p_msg);
    unsigned int max_len = *((unsigned int*)(context_state->args));

    list* route = getRoute(copies, myID, max_len);
    unsigned int size = route->size*sizeof(uuid_t);
    byte buffer[size];
    byte* ptr = buffer;

    for(list_item* it = route->head; it; it = it->next) {
        unsigned char* id = (unsigned char*)(it->data);
        memcpy(ptr, id, sizeof(uuid_t));
        ptr += sizeof(uuid_t);
    }

    appendHeader(serialized_headers, "route", buffer, size);
}

static void RouteContextParseHeaders(ModuleState* context_state, hash_table* serialized_headers, hash_table* headers, unsigned char* myID) {
    list* route = list_init();

    byte* buffer = (byte*)hash_table_find_value(serialized_headers, "route");
    if(buffer) {
        byte* ptr = buffer;

        byte size = 0;
        memcpy(&size, ptr, sizeof(byte));
        ptr += sizeof(byte);

        int n = size / sizeof(uuid_t);
        for(int i = 0; i < n; i++) {
            unsigned char* id = malloc(sizeof(uuid_t));
            uuid_copy(id, ptr);
            ptr += sizeof(uuid_t);

            list_add_item_to_tail(route, id);
        }
    }

    const char* key_ = "route";
    char* key = malloc(strlen(key_)+1);
    strcpy(key, key_);
    hash_table_insert(headers, key, route);
}

static void RouteContextDestroy(ModuleState* context_state) {
    free(context_state->args);
}

RetransmissionContext* RouteContext(unsigned int max_len) {
    assert(max_len > 0);

	unsigned int* max_len_arg = malloc(sizeof(unsigned int));
	*max_len_arg = max_len;

    return newRetransmissionContext(
        "RouteContext",
        max_len_arg,
        NULL,
        NULL,
        NULL,
        &RouteContextAppendHeaders,
        &RouteContextParseHeaders,
        NULL,
        NULL,
        &RouteContextDestroy,
        NULL
    );
}
