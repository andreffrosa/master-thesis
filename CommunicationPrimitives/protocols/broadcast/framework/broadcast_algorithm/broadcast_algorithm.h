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

#ifndef _BROADCAST_ALGORITHM_H_
#define _BROADCAST_ALGORITHM_H_

#include <stdarg.h>

#include "../pending_messages/pending_message.h"

#include "retransmission_context/retransmission_context.h"
#include "retransmission_delay/retransmission_delay.h"
#include "retransmission_policy/retransmission_policy.h"

typedef struct _BroadcastAlgorithm BroadcastAlgorithm;

BroadcastAlgorithm* newBroadcastAlgorithm(RetransmissionContext* r_context, RetransmissionDelay* r_delay, RetransmissionPolicy* r_policy, unsigned int n_phases);

void destroyBroadcastAlgorithm(BroadcastAlgorithm* alg);

RetransmissionContext* getRetransmissionContext(BroadcastAlgorithm* alg);

RetransmissionContext* setRetransmissionContext(BroadcastAlgorithm* alg, RetransmissionContext* new_context);

RetransmissionDelay* getRetransmissionDelay(BroadcastAlgorithm* alg);

RetransmissionDelay* setRetransmissionDelay(BroadcastAlgorithm* alg, RetransmissionDelay* new_delay);

RetransmissionPolicy* getRetransmissionPolicy(BroadcastAlgorithm* alg);

RetransmissionPolicy* setRetransmissionPolicy(BroadcastAlgorithm* alg, RetransmissionPolicy* new_policy);

unsigned int getRetransmissionPhases(BroadcastAlgorithm* alg);

void setRetransmissionPhases(BroadcastAlgorithm* alg, unsigned int phases);

unsigned long triggerRetransmissionDelay(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID);

bool triggerRetransmissionPolicy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID);

unsigned int triggerRetransmissionContextHeader(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, void** context_header, unsigned char* myID);

void triggerRetransmissionContextInit(BroadcastAlgorithm* algorithm, proto_def* protocol_definition, unsigned char* myID);

void triggerRetransmissionContextEvent(BroadcastAlgorithm* algorithm, queue_t_elem* event, unsigned char* myID);

void triggerRetransmissionContextCopy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID);

#endif /* _BROADCAST_ALGORITHM_H_ */
