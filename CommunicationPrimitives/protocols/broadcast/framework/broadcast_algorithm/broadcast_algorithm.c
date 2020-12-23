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

#include <stdlib.h>
#include <assert.h>

#include "broadcast_algorithm_private.h"

#include "utility/my_string.h"

BroadcastAlgorithm* newBroadcastAlgorithm(list* contexts, RetransmissionDelay* r_delay, RetransmissionPolicy* r_policy, unsigned int n_phases) {
	BroadcastAlgorithm* algorithm = (BroadcastAlgorithm*)malloc(sizeof(BroadcastAlgorithm));

	//algorithm->r_context = r_context;

    algorithm->contexts = hash_table_init((hashing_function)&string_hash, (comparator_function)&equal_str);
    algorithm->contexts_order = list_init();

    RetransmissionContext* r_context = NULL;
    while((r_context = list_remove_head(contexts))) {
        BA_addContext(algorithm, r_context);
    }
    free(contexts);

    algorithm->r_delay = NULL;
    BA_setRetransmissionDelay(algorithm, r_delay);

    algorithm->r_policy = NULL;
    BA_setRetransmissionPolicy(algorithm, r_policy);

	algorithm->n_phases = n_phases;

	return algorithm;
}

void BA_addContext(BroadcastAlgorithm* algorithm, RetransmissionContext* r_context) {
    assert(algorithm);

    bool valid = list_contained(RC_getDependencies(r_context), algorithm->contexts_order, &equal_str, true);
    assert(valid);

    const char* context_id = RC_getID(r_context);

    char* id_copy = malloc((strlen(context_id)+1)*sizeof(char));
    strcpy(id_copy, context_id);

    char* id_copy_2 = malloc((strlen(context_id)+1)*sizeof(char));
    strcpy(id_copy_2, context_id);

    void* aux = hash_table_insert(algorithm->contexts, id_copy, r_context);
    assert(aux == NULL);

    list_add_item_to_tail(algorithm->contexts_order, id_copy_2);
}

static void delete_contexts(hash_table_item* hit, void* args) {
    RetransmissionContext* r_context =  hit->value;

    destroyRetransmissionContext(r_context);
}

void destroyBroadcastAlgorithm(BroadcastAlgorithm* algorithm) {
    if(algorithm != NULL) {
        destroyRetransmissionDelay(algorithm->r_delay);

        destroyRetransmissionPolicy(algorithm->r_policy);

        hash_table_delete_custom(algorithm->contexts, &delete_contexts, NULL);
        list_delete(algorithm->contexts_order);

        free(algorithm);
    }
}

RetransmissionContext* BA_getRetransmissionContext(BroadcastAlgorithm* algorithm, const char* context_id) {
    assert(algorithm != NULL);

    return hash_table_find_value(algorithm->contexts, (void*)context_id);
}

void BA_flushRetransmissionContexts(BroadcastAlgorithm* algorithm) {
    assert(algorithm != NULL);

    hash_table_delete_custom(algorithm->contexts, &delete_contexts, NULL);
    list_delete(algorithm->contexts_order);

    algorithm->contexts = hash_table_init((hashing_function)&string_hash, (comparator_function)&equal_str);
    algorithm->contexts_order = list_init();
}

/*
void BA_setRetransmissionContext(BroadcastAlgorithm* algorithm, RetransmissionContext* new_context) {
    assert(algorithm != NULL);

    list* visited = list_init();
    destroyRetransmissionContext(algorithm->r_context, visited);
    list_delete(visited);

    algorithm->r_context = new_context;
}
*/

RetransmissionDelay* BA_getRetransmissionDelay(BroadcastAlgorithm* algorithm) {
    assert(algorithm != NULL);

    return algorithm->r_delay;
}

void BA_setRetransmissionDelay(BroadcastAlgorithm* algorithm, RetransmissionDelay* new_delay) {
    assert(algorithm != NULL && new_delay != NULL);

    bool valid = list_contained(RD_getDependencies(new_delay), algorithm->contexts_order, &equal_str, true);
    assert(valid);

    destroyRetransmissionDelay(algorithm->r_delay);

    algorithm->r_delay = new_delay;
}

RetransmissionPolicy* BA_getRetransmissionPolicy(BroadcastAlgorithm* algorithm) {
    assert(algorithm != NULL);

    return algorithm->r_policy;
}

void BA_setRetransmissionPolicy(BroadcastAlgorithm* algorithm, RetransmissionPolicy* new_policy) {
    assert(algorithm != NULL && new_policy);

    bool valid = list_contained(RP_getDependencies(new_policy), algorithm->contexts_order, &equal_str, true);
    assert(valid);

    destroyRetransmissionPolicy(algorithm->r_policy);

    algorithm->r_policy = new_policy;
}

unsigned int BA_getRetransmissionPhases(BroadcastAlgorithm* algorithm) {
    assert(algorithm != NULL);

    return algorithm->n_phases;
}

void BA_setRetransmissionPhases(BroadcastAlgorithm* algorithm, unsigned int phases) {
    assert(algorithm != NULL);

    algorithm->n_phases = phases;
}

unsigned long BA_computeRetransmissionDelay(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID) {
    assert(algorithm != NULL && p_msg != NULL);

    RetransmissionDelay* r_delay = algorithm->r_delay;

	unsigned long result = RD_compute(r_delay, p_msg, remaining, isCopy, myID, algorithm->contexts);

    return result;
}

bool BA_evalRetransmissionPolicy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID) {
    assert(algorithm != NULL);

    RetransmissionPolicy* r_policy = algorithm->r_policy;

	bool result = RP_eval(r_policy, p_msg, myID, algorithm->contexts);

    return result;
}

void BA_initRetransmissionContext(BroadcastAlgorithm* algorithm, proto_def* protocol_definition, unsigned char* myID) {
    assert(algorithm != NULL);

    for(list_item* it = algorithm->contexts_order->head; it; it = it->next) {
        char* context_id = (char*)it->data;
        RetransmissionContext* rc = (RetransmissionContext*)hash_table_find_value(algorithm->contexts, context_id);

        RC_init(rc, protocol_definition, myID);
    }
}

void BA_processEvent(BroadcastAlgorithm* algorithm, queue_t_elem* event, unsigned char* myID) {
    assert(algorithm != NULL);

    for(list_item* it = algorithm->contexts_order->head; it; it = it->next) {
        char* context_id = (char*)it->data;
        RetransmissionContext* rc = (RetransmissionContext*)hash_table_find_value(algorithm->contexts, context_id);

        RC_processEvent(rc, event, myID, algorithm->contexts);
    }
}

unsigned short BA_createHeader(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, byte** context_header, unsigned char* myID, struct timespec* current_time) {
    assert(algorithm != NULL && p_msg != NULL);

    hash_table* serialized_headers = hash_table_init((hashing_function)&string_hash, (comparator_function)&equal_str);

    for(list_item* it = algorithm->contexts_order->head; it; it = it->next) {
        char* context_id = (char*)it->data;
        RetransmissionContext* rc = (RetransmissionContext*)hash_table_find_value(algorithm->contexts, context_id);

        RC_appendHeaders(rc, p_msg, serialized_headers, myID, algorithm->contexts, current_time);
    }

    unsigned int total_size = 0;

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(serialized_headers, &iterator)) ) {
        if( hit->value != NULL ) {
            unsigned int size = ((byte*)hit->value)[0];
            unsigned int effective_size = (strlen(hit->key) + 1) + sizeof(byte) + size;
            total_size += effective_size;
        }
    }

    assert(total_size <= 255); // Precaution

    byte* buffer = NULL;
    if( total_size > 0 ) {
        buffer = malloc(total_size);
        byte* ptr = buffer;

        void* iterator = NULL;
        hash_table_item* hit = NULL;
        while( (hit = hash_table_iterator_next(serialized_headers, &iterator)) ) {
            if( hit->value != NULL ) {
                char* key = hit->key;
                byte size = ((byte*)hit->value)[0];

                int key_size = strlen(key)+1;
                assert(key_size <= 100);

                memcpy(ptr, key, key_size);
                ptr += key_size;

                memcpy(ptr, hit->value, sizeof(byte) + size);
                ptr += sizeof(byte) + size;
            }
        }
    }

    hash_table_delete(serialized_headers);

    *context_header = buffer;
    return total_size;
}

hash_table* BA_parseHeader(BroadcastAlgorithm* algorithm, unsigned short header_size, byte* context_header, unsigned char* myID) {
    assert(algorithm);

    hash_table* serialized_headers = hash_table_init((hashing_function)&string_hash, (comparator_function)&equal_str);

    byte* ptr = context_header;

    while( ptr < context_header + header_size ) {
        char* key = new_str((char*)ptr);
        int key_size = strlen(key) + 1;
        ptr += key_size;

        byte size = 0;
        memcpy(&size, ptr, sizeof(byte));
        // ptr += sizeof(byte);

        byte* buffer = malloc(sizeof(byte) + size);
        memcpy(buffer, ptr, sizeof(byte) + size);
        ptr += sizeof(byte) + size;

        hash_table_insert(serialized_headers, key, buffer);
    }

    hash_table* headers = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);

    for(list_item* it = algorithm->contexts_order->head; it; it = it->next) {
        char* context_id = (char*)it->data;
        RetransmissionContext* rc = (RetransmissionContext*)hash_table_find_value(algorithm->contexts, context_id);

        RC_parseHeaders(rc, serialized_headers, headers, myID);
    }

    hash_table_delete(serialized_headers);

    return headers;
}

void BA_processCopy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID) {
    assert(algorithm != NULL && p_msg != NULL);

    for(list_item* it = algorithm->contexts_order->head; it; it = it->next) {
        char* context_id = (char*)it->data;
        RetransmissionContext* rc = (RetransmissionContext*)hash_table_find_value(algorithm->contexts, context_id);

        RC_processCopy(rc, p_msg, myID);
    }
}
