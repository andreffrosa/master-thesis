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

#ifndef _RETRANSMISSION_DELAY_H_
#define _RETRANSMISSION_DELAY_H_

#include "../common.h"

#include "../retransmission_context/retransmission_context.h"

typedef struct _RetransmissionDelay RetransmissionDelay;

unsigned long RD_compute(RetransmissionDelay* r_delay, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, hash_table* contexts);

//void RD_addDependency(RetransmissionDelay* r_delay, char* dependency);

list* RD_getDependencies(RetransmissionDelay* r_delay);

void destroyRetransmissionDelay(RetransmissionDelay* r_delay);

////////////////////////////////////////////////////////////////////////////////

RetransmissionDelay* NullDelay();

RetransmissionDelay* RandomDelay(unsigned long t);
RetransmissionDelay* TwoPhaseRandomDelay(unsigned long t1, unsigned long t2);

RetransmissionDelay* SBADelay(unsigned long t);
RetransmissionDelay* DensityNeighDelay(unsigned long t);

RetransmissionDelay* RADExtensionDelay(unsigned long delta_t);
RetransmissionDelay* HopCountAwareRADExtensionDelay(unsigned long delta_t);

RetransmissionDelay* BiFloodingDelay(unsigned long t1, unsigned long t2);

#endif /* _RETRANSMISSION_DELAY_H_ */
