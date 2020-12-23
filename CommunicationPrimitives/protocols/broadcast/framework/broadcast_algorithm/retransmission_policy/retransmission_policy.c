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

RetransmissionPolicy* newRetransmissionPolicy(void* args, void* vars, eval_policy eval, destroy_policy destroy, list* dependencies) {
    assert(eval);

    RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->state.args = args;
	r_policy->state.vars = vars;

	r_policy->eval = eval;
    r_policy->destroy = destroy;

    r_policy->context_dependencies = dependencies == NULL ? list_init() : dependencies;

	return r_policy;
}

bool RP_eval(RetransmissionPolicy* r_policy, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {
    assert(r_policy && p_msg && myID && contexts);

    return r_policy->eval(&r_policy->state, p_msg, myID, contexts);
}

void RP_addDependency(RetransmissionPolicy* r_policy, char* dependency) {
    assert(r_policy && dependency);

    char* d = malloc(strlen(dependency)+1);
    strcpy(d, dependency);
    list_add_item_to_tail(r_policy->context_dependencies, d);
}

list* RP_getDependencies(RetransmissionPolicy* r_policy) {
    assert(r_policy);

    return r_policy->context_dependencies;
}

void destroyRetransmissionPolicy(RetransmissionPolicy* r_policy) {

    if(r_policy != NULL) {
        if(r_policy->destroy != NULL) {
            r_policy->destroy(&r_policy->state);
        }
        list_delete(r_policy->context_dependencies);
        free(r_policy);
    }

}
