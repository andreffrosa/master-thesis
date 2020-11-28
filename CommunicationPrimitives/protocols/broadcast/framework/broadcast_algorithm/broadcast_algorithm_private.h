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
 * (C) 2020
 *********************************************************/

#ifndef _BROADCAST_ALGORITHM_PRIVATE_H_
#define _BROADCAST_ALGORITHM_PRIVATE_H_

#include "broadcast_algorithm.h"

#include "retransmission_context/retransmission_context_private.h"
#include "retransmission_delay/retransmission_delay_private.h"
#include "retransmission_policy/retransmission_policy_private.h"

typedef struct _BroadcastAlgorithm {
	RetransmissionDelay* r_delay;
	RetransmissionPolicy* r_policy;
	RetransmissionContext* r_context;
	unsigned int n_phases;
} BroadcastAlgorithm;

#endif /* _BROADCAST_ALGORITHM_PRIVATE_H_ */
