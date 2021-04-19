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

#ifndef _BROADCAST_FRAMEWORK_H_
#define _BROADCAST_FRAMEWORK_H_

#include "Yggdrasil.h"

#include "data_structures/double_list.h"

#include "pending_messages/pending_messages.h"
#include "broadcast_header.h"

#include "broadcast_algorithm/bcast_algorithms.h"

#define BROADCAST_FRAMEWORK_PROTO_ID 160
#define BROADCAST_FRAMEWORK_PROTO_NAME "BROADCAST FRAMEWORK"

typedef struct broadcast_stats_ {
	unsigned long messages_transmitted;
	unsigned long messages_not_transmitted;
	unsigned long messages_delivered;
	unsigned long messages_received;
	unsigned long messages_bcasted;
} broadcast_stats;

typedef struct broadcast_framework_args_ {
	BroadcastAlgorithm** algorithms;
    unsigned int algorithms_length;
	unsigned long seen_expiration_ms;
	unsigned long gc_interval_s;
    bool late_delivery;
} broadcast_framework_args;

proto_def* broadcast_framework_init(void* arg);
void* broadcast_framework_main_loop(main_loop_args* args);

broadcast_framework_args* new_broadcast_framework_args(BroadcastAlgorithm** algorithms, unsigned int algorithms_length, unsigned long seen_expiration_ms, unsigned long gc_interval_s, bool late_delivery);

broadcast_framework_args* default_broadcast_framework_args();

broadcast_framework_args* load_broadcast_framework_args(const char* file_path);

void BroadcastMessage(unsigned short protocol_id, unsigned short ttl, unsigned int alg, byte* data, unsigned short size);

typedef enum {
    REQ_BROADCAST_MESSAGE = 0,
	REQ_BROADCAST_FRAMEWORK_STATS,
    BROADCAST_REQ_TYPE_COUNT
} BcastRequestType;

typedef enum {
	TIMER_BROADCAST_MESSAGE_TIMEOUT = 0,
	BROADCAST_TIMER_TYPE_COUNT
} BcastTimerType;

typedef enum {
	MSG_BROADCAST_MESSAGE = 0,
	BROADCAST_MSG_TYPE_COUNT
} BcastMessageType;

#endif /* _BROADCAST_FRAMEWORK_H_ */
