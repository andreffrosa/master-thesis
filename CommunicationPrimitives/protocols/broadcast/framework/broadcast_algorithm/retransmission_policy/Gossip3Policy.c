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

static bool _Gossip3Policy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {
	Gossip3PolicyArgs* _args = ((Gossip3PolicyArgs*)(policy_state->args));
	double p = _args->p;
	unsigned int k = _args->k;
	unsigned int m = _args->m;
	message_copy* first = ((message_copy*)getCopies(p_msg)->head->data);
	unsigned char hops = 0;
	if(!query_context_header(r_context, getContextHeader(first), getBcastHeader(first)->context_length, "hops", &hops, myID, 0))
				assert(false);
	unsigned int n_copies = getCopies(p_msg)->size;
	unsigned int current_phase = getCurrentPhase(p_msg);

	if(current_phase == 1) {
		double u = randomProb();
		return hops < k || u <= p;
	} else {
        bool retransmitted = getPhaseDecision(getPhaseStats(p_msg, getCurrentPhase(p_msg)-1));
        return !retransmitted && n_copies < m;
	}
}

static void _Gossip3PolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* Gossip3Policy(double p, unsigned int k, unsigned int m) {
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = malloc(sizeof(Gossip3PolicyArgs));
	Gossip3PolicyArgs* args = ((Gossip3PolicyArgs*)r_policy->policy_state.args);
	args->p = p;
	args->k = k;
	args->m = m;

	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_Gossip3Policy;
    r_policy->destroy = &_Gossip3PolicyDestroy;

	return r_policy;
}
