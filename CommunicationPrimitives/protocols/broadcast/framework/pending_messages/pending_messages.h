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

#ifndef _PENDING_MESSAGES_H_
#define _PENDING_MESSAGES_H_

#include "Yggdrasil/core/ygg_runtime.h"

#include "../bcast_header.h"

#include "data_structures/list.h"
#include "data_structures/double_list.h"

#include "pending_message.h"

typedef struct _PendingMessages PendingMessages;

PendingMessages* newPendingMessages();

int deletePendingMessages(PendingMessages* p_msgs);

void pushPendingMessage(PendingMessages* p_msgs, PendingMessage* p_msg);

PendingMessage* popPendingMessage(PendingMessages* p_msgs, PendingMessage* it);

PendingMessage* getPendingMessage(PendingMessages* p_msgs, uuid_t id);

bool isInSeenMessages(PendingMessages* p_msgs, uuid_t id);

unsigned int runGarbageCollectorPM(PendingMessages* p_msgs, struct timespec* current_time, struct timespec* seen_expiration);

#endif /* _PENDING_MESSAGES_H_ */
