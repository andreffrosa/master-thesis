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

RetransmissionDelay* newRetransmissionDelay(void* args, void* vars, compute_delay compute, destroy_delay destroy, list* dependencies) {
    assert(compute);

    RetransmissionDelay* r_delay = malloc(sizeof(RetransmissionDelay));

	r_delay->state.args = args;
	r_delay->state.vars = vars;
	r_delay->compute = compute;
    r_delay->destroy = destroy;

    r_delay->context_dependencies = dependencies == NULL ? list_init() : dependencies;

	return r_delay;
}

unsigned long RD_compute(RetransmissionDelay* r_delay, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, hash_table* contexts) {
    assert(r_delay && contexts);

    return r_delay->compute(&r_delay->state, p_msg, remaining, isCopy, myID, contexts);
}

void RD_addDependency(RetransmissionDelay* r_delay, char* dependency) {
    assert(r_delay && dependency);

    char* d = malloc(strlen(dependency)+1);
    strcpy(d, dependency);
    list_add_item_to_tail(r_delay->context_dependencies, d);
}

list* RD_getDependencies(RetransmissionDelay* r_delay) {
    assert(r_delay);

    return r_delay->context_dependencies;
}

void destroyRetransmissionDelay(RetransmissionDelay* r_delay) {

    if(r_delay != NULL) {
        if(r_delay->destroy != NULL) {
            r_delay->destroy(&r_delay->state);
        }
        list_delete(r_delay->context_dependencies);
        free(r_delay);
    }
}
