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

BroadcastAlgorithm* newBroadcastAlgorithm(list* contexts, RetransmissionDelay* r_delay, RetransmissionPolicy* r_policy, unsigned int n_phases);

void destroyBroadcastAlgorithm(BroadcastAlgorithm* alg);

RetransmissionContext* BA_getRetransmissionContext(BroadcastAlgorithm* alg, const char* context_id);

//void BA_setRetransmissionContext(BroadcastAlgorithm* alg, RetransmissionContext* new_context);

void BA_addContext(BroadcastAlgorithm* alg, RetransmissionContext* r_context);

void BA_flushRetransmissionContexts(BroadcastAlgorithm* algorithm);

RetransmissionDelay* BA_getRetransmissionDelay(BroadcastAlgorithm* alg);

void BA_setRetransmissionDelay(BroadcastAlgorithm* alg, RetransmissionDelay* new_delay);

RetransmissionPolicy* BA_getRetransmissionPolicy(BroadcastAlgorithm* alg);

void BA_setRetransmissionPolicy(BroadcastAlgorithm* alg, RetransmissionPolicy* new_policy);

unsigned int BA_getRetransmissionPhases(BroadcastAlgorithm* alg);

void BA_setRetransmissionPhases(BroadcastAlgorithm* alg, unsigned int phases);

unsigned long BA_computeRetransmissionDelay(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID);

bool BA_evalRetransmissionPolicy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID);

void BA_initRetransmissionContext(BroadcastAlgorithm* algorithm, proto_def* protocol_definition, unsigned char* myID);

void BA_processEvent(BroadcastAlgorithm* algorithm, queue_t_elem* event, unsigned char* myID);

unsigned short BA_createHeader(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, byte** context_header, unsigned char* myID, struct timespec* current_time);

hash_table* BA_parseHeader(BroadcastAlgorithm* algorithm, unsigned short header_size, byte* context_header, unsigned char* myID);

void BA_processCopy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID);

#endif /* _BROADCAST_ALGORITHM_H_ */
