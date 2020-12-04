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


static unsigned int LatencyContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, unsigned char* myID, list* visited) {

    unsigned long* t = malloc(sizeof(unsigned long));

    assert(getCopies(p_msg)->size > 0);

    //if (getCopies(p_msg)->size > 0) {
        struct timespec* reception_time = getCopyReceptionTime(((MessageCopy*)getCopies(p_msg)->head->data));
        struct timespec current_time = {0}, elapsed = {0};
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        subtract_timespec(&elapsed, &current_time, reception_time);

        *t = timespec_to_milli(&elapsed);
    /*} else {
        *t = 0L;
    }*/

	*context_header = t;
	return sizeof(unsigned long);
}

static bool LatencyContextQueryHeader(ModuleState* context_state, void* header, unsigned int header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {

    if(strcmp(query, "latency") == 0) {
        if(header) {
            *((unsigned long*)result) = *((unsigned long*)header);
        } else {
            *((unsigned long*)result) = 0L;
        }

		return true;
	}

	return false;
}

static void LatencyContextCopy(ModuleState* context_state, PendingMessage* p_msg, unsigned char* myID, list* visited) {

    if(getCurrentPhase(p_msg) == 1 && getCopies(p_msg)->size == 1) {
        MessageCopy* first = ((MessageCopy*)getCopies(p_msg)->head->data);

        unsigned long latency = 0L;
        if(!LatencyContextQueryHeader(context_state, getContextHeader(first), getBcastHeader(first)->context_length, "latency", &latency, NULL, myID, NULL))
    		assert(false);

        // Debug
        char str[100];
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(getPendingMessageID(p_msg), id_str);
        sprintf(str, "[%s] latency = %lu", id_str, latency);
        ygg_log("LATENCY CONTEXT", "LATENCY", str);
    }
}

RetransmissionContext* LatencyContext() {

    return newRetransmissionContext(
        NULL,
        NULL,
        NULL,
        NULL,
        &LatencyContextHeader,
        NULL,
        &LatencyContextQueryHeader,
        &LatencyContextCopy,
        NULL
    );
}
