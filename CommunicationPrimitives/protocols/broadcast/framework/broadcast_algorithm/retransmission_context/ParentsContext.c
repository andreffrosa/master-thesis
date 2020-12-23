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
 * (C) 2019
 *********************************************************/

#include "retransmission_context_private.h"

#include <assert.h>

#include "utility/my_math.h"


static void ParentsContextAppendHeaders(ModuleState* context_state, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time) {
    double_list* copies = getCopies(p_msg);
    assert(copies->size > 0);

    unsigned int max_amount = *((unsigned int*)(context_state->args));
    unsigned int real_amount = iMin(max_amount, copies->size);
    unsigned int size = real_amount*sizeof(uuid_t);

    byte buffer[size];
    byte* ptr = buffer;

    double_list_item* dit = copies->head;
	for(int i = 0; i < real_amount; i++, dit = dit->next) {
        MessageCopy* copy = (MessageCopy*)dit->data;

    	uuid_copy(ptr, getBcastHeader(copy)->sender_id);
        ptr += sizeof(uuid_t);
    }

    appendHeader(serialized_headers, "parents", buffer, size);
}

static void ParentsContextParseHeaders(ModuleState* context_state, hash_table* serialized_headers, hash_table* headers, unsigned char* myID) {
    list* parents = list_init();

    byte* buffer = (byte*)hash_table_find_value(serialized_headers, "parents");
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
            list_add_item_to_tail(parents, id);
        }
    }

    const char* key_ = "parents";
    char* key = malloc(strlen(key_)+1);
    strcpy(key, key_);
    hash_table_insert(headers, key, parents);
}

static void ParentsContextDestroy(ModuleState* context_state) {
    free(context_state->args);
}

RetransmissionContext* ParentsContext(unsigned int max_amount) {
    assert( 0 < max_amount && max_amount <= 255);

    unsigned int* max_amount_arg = malloc(sizeof(unsigned int));
    *max_amount_arg = max_amount;

    return newRetransmissionContext(
        "ParentsContext",
        max_amount_arg,
        NULL,
        NULL,
        NULL,
        &ParentsContextAppendHeaders,
        &ParentsContextParseHeaders,
        NULL,
        NULL,
        &ParentsContextDestroy,
        NULL
    );
}
