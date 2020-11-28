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

#include "../common.h"

#include "../retransmission_context/retransmission_context.h"

typedef struct _RetransmissionDelay {
    ModuleState delay_state;
	unsigned long (*r_delay)(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, RetransmissionContext* r_context, unsigned char* myID);
    void (*destroy)(ModuleState* context_state, list* visited);
} RetransmissionDelay;

#endif /* _RETRANSMISSION_DELAY_PRIVATE_H_ */
