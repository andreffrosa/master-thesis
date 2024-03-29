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

typedef struct _Gossip3PolicyArgs {
	double p;
	unsigned int k;
	unsigned int m;
} Gossip3PolicyArgs;

static bool Gossip3PolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {
	Gossip3PolicyArgs* args = ((Gossip3PolicyArgs*)(policy_state->args));
	double p = args->p;
	unsigned int k = args->k;
	unsigned int m = args->m;

	MessageCopy* first = ((MessageCopy*)getCopies(p_msg)->head->data);
    hash_table* headers = getHeaders(first);

	unsigned int n_copies = getCopies(p_msg)->size;
	unsigned int current_phase = getCurrentPhase(p_msg);

	if(current_phase == 1) {
        byte* hops = (byte*)hash_table_find_value(headers, "hops");
        assert(hops);

		double u = randomProb();
		return *hops < k || u <= p;
	} else {
        bool retransmitted = getPhaseDecision(getPhaseStats(p_msg, getCurrentPhase(p_msg)-1));
        return !retransmitted && n_copies < m;
	}
}

static void Gossip3PolicyDestroy(ModuleState* policy_state) {
    free(policy_state->args);
}

RetransmissionPolicy* Gossip3Policy(double p, unsigned int k, unsigned int m) {
    assert( 0.0 < p && p <= 1.0);

	Gossip3PolicyArgs* args = malloc(sizeof(Gossip3PolicyArgs));
	args->p = p;
	args->k = k;
	args->m = m;

    RetransmissionPolicy* r_policy = newRetransmissionPolicy(
        args,
        NULL,
        &Gossip3PolicyEval,
        &Gossip3PolicyDestroy,
        new_list(1, new_str("HopsContext"))
    );

    return r_policy;
}
