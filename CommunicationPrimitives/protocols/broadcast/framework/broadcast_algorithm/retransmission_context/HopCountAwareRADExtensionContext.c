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

static void HCA_RADExtension_AppendHeaders(ModuleState* context_state, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time) {
    unsigned long delta_t = *((unsigned long*) (context_state->args));

    unsigned long delay = getCurrentPhaseDuration(p_msg);

    double_list* copies = getCopies(p_msg);
    unsigned char n_copies = copies->size;
    assert(n_copies > 0);
    MessageCopy* first = ((MessageCopy*)copies->head->data);
    hash_table* headers = getHeaders(first);

    unsigned long* parent_initial_delay = (unsigned long*)hash_table_find_value(headers, "delay");

    unsigned long initial_delay = delay - 2*n_copies*delta_t - (delta_t - (parent_initial_delay?*parent_initial_delay:0));

    appendHeader(serialized_headers, "delay", &initial_delay, sizeof(unsigned long));
}

static void HCA_RADExtension_ParseHeaders(ModuleState* context_state, hash_table* serialized_headers, hash_table* headers, unsigned char* myID) {

    byte* buffer = (byte*)hash_table_find_value(serialized_headers, "delay");
    if(buffer) {
        byte* ptr = buffer;

        byte size = 0;
        memcpy(&size, ptr, sizeof(byte));
        ptr += sizeof(byte);

        unsigned long* delay = malloc(sizeof(unsigned long));
        memcpy(delay, ptr, sizeof(unsigned long));

        const char* key_ = "delay";
        char* key = malloc(strlen(key_)+1);
        strcpy(key, key_);
        hash_table_insert(headers, key, delay);
    }
}

static void HopCountAwareRADExtensionContextDestroy(ModuleState* context_state) {
    free(context_state->args);
}

RetransmissionContext* HopCountAwareRADExtensionContext(unsigned long delta_t) {
    assert(delta_t > 0);

    unsigned long* delta_t_arg = malloc(sizeof(unsigned long));
    *delta_t_arg = delta_t;

    return newRetransmissionContext(
        "HopCountAwareRADExtensionContext",
        delta_t_arg,
        NULL,
        NULL,
        NULL,
        &HCA_RADExtension_AppendHeaders,
        &HCA_RADExtension_ParseHeaders,
        NULL,
        NULL,
        &HopCountAwareRADExtensionContextDestroy,
        NULL
    );
}
