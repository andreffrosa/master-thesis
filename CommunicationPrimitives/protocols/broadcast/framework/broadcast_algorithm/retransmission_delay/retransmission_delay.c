/*********************************************************
 * This code was written in the r_policy of the Lightkone
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

#include "retransmission_delay_private.h"

#include <assert.h>

RetransmissionDelay* newRetransmissionDelay(void* args, void* vars, compute_delay compute, destroy_delay destroy) {
    assert(compute);

    RetransmissionDelay* r_delay = malloc(sizeof(RetransmissionDelay));

	r_delay->state.args = args;
	r_delay->state.vars = vars;
	r_delay->compute = compute;
    r_delay->destroy = destroy;

	return r_delay;
}

unsigned long RD_compute(RetransmissionDelay* r_delay, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
    assert(r_delay && visited);

    if( list_find_item(visited, &equalAddr, r_delay) == NULL ) {
        void** this = malloc(sizeof(void*));
        *this = r_delay;
        list_add_item_to_tail(visited, this);

        return r_delay->compute(&r_delay->state, p_msg, remaining, isCopy, myID, r_context, visited);
    } else {
        return 0L;
        // TODO: what to do?
    }
}

void destroyRetransmissionDelay(RetransmissionDelay* r_delay, list* visited) {
    assert(visited);

    if(r_delay != NULL) {
        if( list_find_item(visited, &equalAddr, r_delay) == NULL ) {
            void** this = malloc(sizeof(void*));
            *this = r_delay;
            list_add_item_to_tail(visited, this);

            if(r_delay->destroy != NULL) {
                r_delay->destroy(&r_delay->state, visited);
            }
            free(r_delay);
        }
    }
}
