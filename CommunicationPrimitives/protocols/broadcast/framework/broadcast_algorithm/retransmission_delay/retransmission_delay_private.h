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

#ifndef _RETRANSMISSION_DELAY_PRIVATE_H_
#define _RETRANSMISSION_DELAY_PRIVATE_H_

#include "retransmission_delay.h"

typedef unsigned long (*compute_delay)(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, hash_table* contexts);

typedef void (*destroy_delay)(ModuleState* context_state);

typedef struct _RetransmissionDelay {
    ModuleState state;
	compute_delay compute;
    destroy_delay  destroy;

    list* context_dependencies;
} RetransmissionDelay;

RetransmissionDelay* newRetransmissionDelay(void* args, void* vars, compute_delay compute, destroy_delay destroy, list* dependencies);

#endif /* _RETRANSMISSION_DELAY_PRIVATE_H_ */
