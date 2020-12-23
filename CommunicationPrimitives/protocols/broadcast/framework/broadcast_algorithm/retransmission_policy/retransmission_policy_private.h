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

#ifndef _RETRANSMISSION_POLICY_PRIVATE_H_
#define _RETRANSMISSION_POLICY_PRIVATE_H_

#include "retransmission_policy.h"

typedef bool (*eval_policy)(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts);

typedef void (*destroy_policy)(ModuleState* context_state);

typedef struct _RetransmissionPolicy {
    ModuleState state;
	eval_policy eval;
    destroy_policy destroy;

    list* context_dependencies;
} RetransmissionPolicy;

RetransmissionPolicy* newRetransmissionPolicy(void* args, void* vars, eval_policy eval, destroy_policy destroy, list* dependencies);

#endif /* _RETRANSMISSION_POLICY_PRIVATE_H_ */
