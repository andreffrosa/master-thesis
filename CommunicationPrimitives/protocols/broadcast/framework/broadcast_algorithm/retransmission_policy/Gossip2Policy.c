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

static bool Gossip2PolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {
	Gossip2PolicyArgs* _args = ((Gossip2PolicyArgs*)(policy_state->args));
	double p1 = _args->p1;
	unsigned int k = _args->k;
	double p2 = _args->p2;
	unsigned int n = _args->n;
    unsigned int nn = 0;

	MessageCopy* first = ((MessageCopy*)getCopies(p_msg)->head->data);
    hash_table* headers = getHeaders(first);

    byte* hops = (byte*)hash_table_find_value(headers, "hops");
    assert(hops);

    RetransmissionContext* r_context = hash_table_find_value(contexts, "NeighborsContext");
    assert(r_context);

    if(!RC_query(r_context, "n_neighbors", &nn, NULL, myID, contexts))
        assert(false);

	double u = randomProb();
	return *hops < k || (nn < n && u <= p2) || (nn >= n && u <= p1);
}

static void Gossip2PolicyDestroy(ModuleState* policy_state) {
    free(policy_state->args);
}

RetransmissionPolicy* Gossip2Policy(double p1, unsigned int k, double p2, unsigned int n) {
	assert( p2 > p1 );

	Gossip2PolicyArgs* args = malloc(sizeof(Gossip2PolicyArgs));
	args->p1 = p1;
	args->k = k;
	args->p2 = p2;
	args->n = n;

    RetransmissionPolicy* r_policy = newRetransmissionPolicy(
        args,
        NULL,
        &Gossip2PolicyEval,
        &Gossip2PolicyDestroy,
        new_list(2, new_str("NeighborsContext"), new_str("HopsContext"))
    );

    return r_policy;
}
