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

static bool HopsContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {

	if(strcmp(query, "hops") == 0) {
        if(context_header) {
            *((unsigned char*)result) = *((unsigned char*)context_header);
        } else {
            *((unsigned char*)result) = 0; // Broadcast source node
        }

		return true;
	}

	return false;
}

static unsigned int HopsContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, unsigned char* myID, list* visited) {
	unsigned int size = sizeof(unsigned char);
	*context_header = malloc(size);

    double_list* copies = getCopies(p_msg);

    assert(copies->size > 0);

	/*if(copies->size == 0) { // // Broadcast source node
		*((unsigned char*)*context_header) = 0;
	} else {*/
		MessageCopy* msg_copy = (MessageCopy*)copies->head->data;

		unsigned char hops = 0;
        if(!HopsContextQueryHeader(context_state, getContextHeader(msg_copy), getBcastHeader(msg_copy)->context_length, "hops", &hops, NULL, myID, visited))
			assert(false);

        hops++;
        memcpy(*context_header, &hops, size);
	//}

	return size;
}

RetransmissionContext* HopsContext() {

    return newRetransmissionContext(
        NULL,
        NULL,
        NULL,
        NULL,
        &HopsContextHeader,
        NULL,
        &HopsContextQueryHeader,
        NULL,
        NULL
    );
}
