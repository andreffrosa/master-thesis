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

#include "retransmission_policy_private.h"

#include "utility/my_math.h"

#include <assert.h>

typedef struct _Gossip2PolicyArgs {
	double p1;
	unsigned int k;
	double p2;
	unsigned int n;
} Gossip2PolicyArgs;

static bool _Gossip2Policy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {
	Gossip2PolicyArgs* _args = ((Gossip2PolicyArgs*)(policy_state->args));
	double p1 = _args->p1;
	unsigned int k = _args->k;
	double p2 = _args->p2;
	unsigned int n = _args->n;
	unsigned char hops = 0;
	message_copy* first = ((message_copy*)getCopies(p_msg)->head->data);
	if(!query_context_header(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "hops", &hops, myID, 0))
		assert(false);
	
    unsigned int nn = 0;
	if(!query_context(r_context, "n_neighbors", &nn, myID, 0))
		assert(false);

	double u = randomProb();

	return hops < k || (nn < n && u <= p2) || (nn >= n && u <= p1);
}

static void _Gossip2PolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* Gossip2Policy(double p1, unsigned int k, double p2, unsigned int n) {

	assert( p2 > p1 );

	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = malloc(sizeof(Gossip2PolicyArgs));
	Gossip2PolicyArgs* args = ((Gossip2PolicyArgs*)(r_policy->policy_state.args));
	args->p1 = p1;
	args->k = k;
	args->p2 = p2;
	args->n = n;

	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_Gossip2Policy;
    r_policy->destroy = &_Gossip2PolicyDestroy;

	return r_policy;
}
