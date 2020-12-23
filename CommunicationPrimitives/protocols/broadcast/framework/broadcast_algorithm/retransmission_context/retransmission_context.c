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

RetransmissionContext* newRetransmissionContext(const char* context_id, void* args, void* vars, init_ctx init, ctx_event_handler process_event, append_ctx_header append_headers, parse_ctx_header parse_headers,  ctx_query_handler query_handler, ctx_copy_handler copy_handler, destroy_ctx destroy, list* dependencies) {
    RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

    r_context->context_id = context_id;

	r_context->state.args = args;
	r_context->state.vars = vars;

	r_context->init = init;
	r_context->append_headers = append_headers;
    r_context->parse_headers = parse_headers;
	r_context->process_event = process_event;
	r_context->query_handler = query_handler;
	//r_context->query_header_handler = query_header_handler;
    r_context->copy_handler = copy_handler;
    r_context->destroy = destroy;

    r_context->dependencies = (dependencies != NULL ? dependencies : list_init());
    //r_context->dependencies =  list_init();

	return r_context;
}

void RC_init(RetransmissionContext* context, proto_def* protocol_definition, unsigned char* myID) {
    assert(context);

    if( context->init ) {
        context->init(&context->state, protocol_definition, myID);
    }
}

void RC_processEvent(RetransmissionContext* context, queue_t_elem* elem, unsigned char* myID, hash_table* contexts) {
    assert(context);

    if( context->process_event ) {
        context->process_event(&context->state, elem, myID, contexts);
    }
}

/*
unsigned short RC_createHeader(RetransmissionContext* context, PendingMessage* p_msg, byte** context_header, unsigned char* myID) {
    assert(context);

    if( context->create_header ) {
        return context->create_header(&context->state, p_msg, context_header, myID);
    } else {
        *context_header = NULL;
        return 0;
    }
}
*/



void RC_appendHeaders(RetransmissionContext* context, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time) {
    assert(context);

    if( context->append_headers ) {
        context->append_headers(&context->state, p_msg, serialized_headers, myID, contexts, current_time);
    }
}

void RC_parseHeaders(RetransmissionContext* context, hash_table* serialized_headers, hash_table* headers, unsigned char* myID) {
    assert(context);

    if( context->parse_headers ) {
        context->parse_headers(&context->state, serialized_headers, headers, myID);
    }
}

bool RC_query(RetransmissionContext* context, const char* query, void* result, hash_table* query_args, unsigned char* myID, hash_table* contexts) {
    assert(context && result && contexts);

    if( context->query_handler ) {
        return context->query_handler(&context->state, query, result, query_args, myID, contexts);
    } else {
        //*result = NULL;
        return false;
    }
}

/*
bool RC_queryHeader(RetransmissionContext* context, void* context_header, unsigned int context_header_size, const char* query, void* result, hash_table* args, unsigned char* myID) {
    assert(context && visited && result);

    if( list_find_item(visited, &equalAddr, context) == NULL ) {
        void** this = malloc(sizeof(void*));
        *this = context;
        list_add_item_to_tail(visited, this);

        if( context->query_header_handler ) {
            return context->query_header_handler(&context->state, context_header, context_header_size, query, result, args, myID, visited);
        } else {
            // *result = NULL;
            return false;
        }
    } else {
        // *result = NULL;
        return false;
    }
}
*/

void RC_processCopy(RetransmissionContext* context, PendingMessage* p_msg, unsigned char* myID) {
    assert(context);

    if( context->copy_handler ) {
        context->copy_handler(&context->state, p_msg, myID);
    }
}

void destroyRetransmissionContext(RetransmissionContext* context) {

    if(context != NULL) {
        if(context->destroy != NULL) {
            context->destroy(&context->state);
        }
        list_delete(context->dependencies);
        free(context);
    }

}

const char* RC_getID(RetransmissionContext* context) {
    assert(context);

    return context->context_id;
}

list* RC_getDependencies(RetransmissionContext* context) {
    assert(context);

    return context->dependencies;
}

void RC_addDependency(RetransmissionContext* context, char* dependency) {
    assert(context && dependency);

    char* d = malloc(strlen(dependency)+1);
    strcpy(d, dependency);
    list_add_item_to_tail(context->dependencies, d);
}


/*


bool query_context(RetransmissionContext* r_context, char* query, void* result, unsigned char* myID, int argc, ...) {
	va_list args;
	va_start(args, argc);

    list* visited = list_init();
    void** this = malloc(sizeof(void*));
    *this = r_context;
    list_add_item_to_tail(visited, this);

	bool r = r_context->query_handler(&r_context->state, query, result, argc, &args, r_context, myID, visited);

    list_delete(visited);

	va_end(args);
	return r;
}

bool query_context_header(RetransmissionContext* r_context, void* context_header, unsigned int context_length, char* query, void* result, unsigned char* myID, int argc, ...) {
	va_list args;
	va_start(args, argc);

    list* visited = list_init();
    void** this = malloc(sizeof(void*));
    *this = r_context;
    list_add_item_to_tail(visited, this);

	bool r = r_context->query_header_handler(&r_context->state, context_header, context_length, query, result, argc, &args, r_context, myID, visited);

    list_delete(visited);

	va_end(args);
	return r;
}

*/


void appendHeader(hash_table* serialized_headers, char* key, void* value, unsigned int size) {
    assert(0 < size && size <= 255);

    byte* buffer = malloc(sizeof(byte) + size);
    byte* ptr = buffer;

    byte size_ = size;
    memcpy(ptr, &size_, size);
    ptr += sizeof(byte);

    memcpy(ptr, value, size);
    ptr += size;

    char* key_ = malloc(strlen(key)+1);
    strcpy(key_, key);
    void* aux = hash_table_insert(serialized_headers, key_, buffer);
    assert(aux == NULL);
}
