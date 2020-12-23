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

typedef struct EnsemblePolicyArgs_ {
	int amount;
	bool (*ensemble_function)(bool* values, int amount);
} EnsemblePolicyArgs;

static bool EnsemblePolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {
	RetransmissionPolicy** policies = ((RetransmissionPolicy**)(policy_state->vars));
	EnsemblePolicyArgs* args = ((EnsemblePolicyArgs*)(policy_state->args));

	bool values[args->amount];
	for(int i = 0; i < args->amount; i++) {
		values[i] = RP_eval(policies[i], p_msg, myID, contexts);
	}

	return args->ensemble_function(values, args->amount);
}

static void EnsemblePolicyDestroy(ModuleState* policy_state) {
    EnsemblePolicyArgs* args = policy_state->vars;
    RetransmissionPolicy** policies = policy_state->vars;

    for(int i = 0; i < args->amount; i++) {
        if(policies[i]->destroy != NULL) {
            destroyRetransmissionPolicy(policies[i]);
        }
	}
    free(policies);
    free(args);
}

RetransmissionPolicy* EnsemblePolicy(bool (*ensemble_function)(bool* values, int amount), int amount, ...) {
	assert(amount >= 1);
	assert(amount == 1 || ensemble_function != NULL);

    list* dependencies = list_init();

	RetransmissionPolicy** policies = malloc(amount*sizeof(RetransmissionPolicy*));
	va_list p_args;
	va_start(p_args, amount);
	for(int i = 0; i < amount; i++) {
		policies[i] = va_arg(p_args, RetransmissionPolicy*);

        list* sub_dependencies = RP_getDependencies(policies[i]);
        for(list_item* it = sub_dependencies->head; it; it = it->next) {
            list_add_item_to_tail(dependencies, new_str((char*)it->data));
        }
	}
	va_end(p_args);

	EnsemblePolicyArgs* args = malloc(sizeof(EnsemblePolicyArgs));
	args->amount = amount;
	args->ensemble_function = ensemble_function;

    RetransmissionPolicy* r_policy = newRetransmissionPolicy(
        args,
        policies,
        &EnsemblePolicyEval,
        &EnsemblePolicyDestroy,
        dependencies
    );

    return r_policy;
}
