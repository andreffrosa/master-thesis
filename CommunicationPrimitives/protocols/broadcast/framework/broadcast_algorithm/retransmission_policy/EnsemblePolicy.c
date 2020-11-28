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

#include <assert.h>

typedef struct _EnsemblePolicyArgs {
	int amount;
	bool (*ensemble_function)(bool* values, int amount);
} EnsemblePolicyArgs;

static bool _EnsemblePolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {
	RetransmissionPolicy** policies = ((RetransmissionPolicy**)(policy_state->vars));
	EnsemblePolicyArgs* args = ((EnsemblePolicyArgs*)(policy_state->args));

	bool values[args->amount];
	for(int i = 0; i < args->amount; i++) {
		values[i] = policies[i]->r_policy(&policies[i]->policy_state, p_msg, r_context, myID);
	}

	return args->ensemble_function(values, args->amount);
}

static void _EnsemblePolicyDestroy(ModuleState* policy_state, list* visited) {
    EnsemblePolicyArgs* args = policy_state->vars;
    RetransmissionPolicy** policies = policy_state->vars;

    for(int i = 0; i < args->amount; i++) {
        if(policies[i]->destroy != NULL) {
            if(list_find_item(visited, &equalAddr, policies[i]) == NULL) {
                void** this = malloc(sizeof(void*));
                *this = policies[i];
                list_add_item_to_tail(visited, this);

                destroyRetransmissionPolicy(policies[i], visited);
            }
        }
	}
    free(policies);
    free(args);
}

RetransmissionPolicy* EnsemblePolicy(bool (*ensemble_function)(bool* values, int amount), int amount, ...) {

	assert(amount >= 1);
	assert(amount == 1 || ensemble_function != NULL);

	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));
	RetransmissionPolicy** policies = malloc(amount*sizeof(RetransmissionPolicy*));
	va_list p_args;
	va_start(p_args, amount);
	for(int i = 0; i < amount; i++) {
		policies[i] = va_arg(p_args, RetransmissionPolicy*);
	}
	va_end(p_args);
	r_policy->policy_state.vars = policies;

	EnsemblePolicyArgs* args = malloc(sizeof(EnsemblePolicyArgs));
	args->amount = amount;
	args->ensemble_function = ensemble_function;
	r_policy->policy_state.args = args;

	r_policy->r_policy = &_EnsemblePolicy;
    r_policy->destroy = &_EnsemblePolicyDestroy;

	return r_policy;
}
