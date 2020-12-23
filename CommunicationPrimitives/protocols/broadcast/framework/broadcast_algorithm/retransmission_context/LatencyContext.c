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

#include "utility/my_time.h"

#include <assert.h>


static unsigned long getLatency(double_list* copies, struct timespec* current_time) {
    assert(copies->size > 0);

    MessageCopy* first = (MessageCopy*)copies->head->data;

    struct timespec* reception_time = getCopyReceptionTime(first);
    struct timespec elapsed_t = {0};
    subtract_timespec(&elapsed_t, current_time, reception_time);

    unsigned long elapsed = timespec_to_milli(&elapsed_t);

    /* char str[100];
    sprintf(str, "%lu", elapsed);
    ygg_log("", "ELAPSED", str); */

    hash_table* headers = getHeaders(first);
    unsigned long* latency = (unsigned long*)hash_table_find_value(headers, "latency");
    if(latency != NULL) {
        return elapsed + *latency;
    } else {
        return elapsed;
    }
}

static void LatencyAppendHeaders(ModuleState* context_state, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time) {
    unsigned long latency = getLatency(getCopies(p_msg), current_time);

    appendHeader(serialized_headers, "latency", &latency, sizeof(unsigned long));
}

static void LatencyContextParseHeaders(ModuleState* context_state, hash_table* serialized_headers, hash_table* headers, unsigned char* myID) {
    byte* buffer = (byte*)hash_table_find_value(serialized_headers, "latency");
    if(buffer) {
        byte* ptr = buffer + sizeof(byte);

        unsigned long* latency = malloc(sizeof(unsigned long));
        memcpy(latency, ptr, sizeof(unsigned long));

        const char* key_ = "latency";
        char* key = malloc(strlen(key_)+1);
        strcpy(key, key_);
        hash_table_insert(headers, key, latency);
    }
}

static void LatencyContextCopy(ModuleState* context_state, PendingMessage* p_msg, unsigned char* myID) {

    if(getCurrentPhase(p_msg) == 1 && getCopies(p_msg)->size == 1) {
        MessageCopy* first = ((MessageCopy*)getCopies(p_msg)->head->data);
        hash_table* headers = getHeaders(first);

        unsigned long* latency = (unsigned long*)hash_table_find_value(headers, "latency");
        assert(latency);

        // Debug
        char str[100];
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(getPendingMessageID(p_msg), id_str);
        sprintf(str, "[%s] %lu", id_str, *latency);
        ygg_log("BROADCAST", "LATENCY", str);
    }
}

RetransmissionContext* LatencyContext() {

    return newRetransmissionContext(
        "LatencyContext",
        NULL,
        NULL,
        NULL,
        NULL,
        &LatencyAppendHeaders,
        &LatencyContextParseHeaders,
        NULL,
        &LatencyContextCopy,
        NULL,
        NULL
    );
}
