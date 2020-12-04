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

#include "retransmission_policy_private.h"

#include <assert.h>

RetransmissionPolicy* newRetransmissionPolicy(void* args, void* vars, eval_policy eval, destroy_policy destroy) {
    assert(eval);

    RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->state.args = args;
	r_policy->state.vars = vars;

	r_policy->eval = eval;
    r_policy->destroy = destroy;

	return r_policy;
}

bool RP_eval(RetransmissionPolicy* r_policy, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
    assert(r_policy && p_msg && myID && visited);

    if( list_find_item(visited, &equalAddr, r_policy) == NULL ) {
        void** this = malloc(sizeof(void*));
        *this = r_policy;
        list_add_item_to_tail(visited, this);

        return r_policy->eval(&r_policy->state, p_msg, myID, r_context, visited);
    } else {
        return false;
        // TODO: what to do?
    }

}

void destroyRetransmissionPolicy(RetransmissionPolicy* r_policy, list* visited) {
    assert(visited);

    if(r_policy != NULL) {
        if( list_find_item(visited, &equalAddr, r_policy) == NULL ) {
            void** this = malloc(sizeof(void*));
            *this = r_policy;
            list_add_item_to_tail(visited, this);

            if(r_policy->destroy != NULL) {
                r_policy->destroy(&r_policy->state, visited);

            }
            free(r_policy);
        }
    }
}
