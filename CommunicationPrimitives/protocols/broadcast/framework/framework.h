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

#ifndef _BCAST_FRAMEWORK_H_
#define _BCAST_FRAMEWORK_H_

#include "Yggdrasil.h"

#include "data_structures/double_list.h"

#include "pending_messages/pending_messages.h"
#include "bcast_header.h"

#include "broadcast_algorithm/bcast_algorithms.h"

#define BCAST_FRAMEWORK_PROTO_ID 160
#define BCAST_FRAMEWORK_PROTO_NAME "BROADCAST FRAMEWORK"

typedef struct _broadcast_stats {
	unsigned int messages_transmitted;
	unsigned int messages_not_transmitted;
	unsigned int messages_delivered;
	unsigned int messages_received;
	unsigned int messages_bcasted;
} broadcast_stats;

typedef struct _broadcast_framework_args {
	BroadcastAlgorithm* algorithm;
	unsigned long seen_expiration_ms;
	unsigned long gc_interval_s;
} broadcast_framework_args;

proto_def* broadcast_framework_init(void* arg);
void* broadcast_framework_main_loop(main_loop_args* args);

broadcast_framework_args* new_broadcast_framework_args(BroadcastAlgorithm* algorithm, unsigned long seen_expiration_ms, unsigned long gc_interval_s);

void BroadcastMessage(short protocol_id, unsigned char* data, unsigned int size, unsigned short ttl);

typedef enum {
    REQ_BROADCAST_MESSAGE = 0,
	REQ_BCAST_FRAMEWORK_STATS,
    BCAST_REQ_TYPE_COUNT
} BcastRequestType;

typedef enum {
	TIMER_BCAST_MESSAGE_TIMEOUT = 0,
	BCAST_TIMER_TYPE_COUNT
} BcastTimerType;

typedef enum {
	MSG_BROADCAST_MESSAGE = 0,
	BCAST_MSG_TYPE_COUNT
} BcastMessageType;

#endif /* _BCAST_FRAMEWORK_H_ */
