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

#include "utility/my_math.h"
#include "utility/my_string.h"

#include <assert.h>

static byte getHops(const char* type, double_list* copies) {
    assert(copies && copies->size > 0);

    byte hops = 0;

    if( strcmp(type, "first") == 0 ) {
        MessageCopy* first = (MessageCopy*)copies->head->data;
        hash_table* headers = getHeaders(first);

        byte* hops_ = (byte*)hash_table_find_value(headers, "hops");
        if(hops_) {
            hops = *hops_;
        }
    } else {
        for(double_list_item* dit = copies->head; dit; dit = dit->next) {
            MessageCopy* copy = (MessageCopy*)dit->data;
            hash_table* headers = getHeaders(copy);

            byte* hops_ = (byte*)hash_table_find_value(headers, "hops");
            if(hops_) {
                if( strcmp(type, "max") == 0 ) {
                    hops = iMax(hops, *hops_);
                } else if( strcmp(type, "min") == 0 ) {
                    hops = iMin(hops, *hops_);
                }
            }
        }
    }

    return hops + 1;
}

static void HopsContextAppendHeaders(ModuleState* context_state, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time) {
    char* type = (char*)context_state->args;

    byte hops = getHops(type, getCopies(p_msg));

    appendHeader(serialized_headers, "hops", &hops, sizeof(byte));
}

static void HopsContextParseHeaders(ModuleState* context_state, hash_table* serialized_headers, hash_table* headers, unsigned char* myID) {

    byte* buffer = (byte*)hash_table_find_value(serialized_headers, "hops");
    if(buffer) {
        byte* ptr = buffer + sizeof(byte);

        byte* hops = malloc(sizeof(byte));
        memcpy(hops, ptr, sizeof(byte));

        const char* key_ = "hops";
        char* key = malloc(strlen(key_)+1);
        strcpy(key, key_);
        hash_table_insert(headers, key, hops);
    }
}

RetransmissionContext* HopsContext(char* type) {

    assert(strcmp(type, "first") == 0 || strcmp(type, "min") == 0 || strcmp(type, "max") == 0);

    char* type_ = new_str(type);

    return newRetransmissionContext(
        "HopsContext",
        type_,
        NULL,
        NULL,
        NULL,
        &HopsContextAppendHeaders,
        &HopsContextParseHeaders,
        NULL,
        NULL,
        NULL,
        NULL
    );
}
