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

#ifndef _ROUTING_FRAMEWORK_H_
#define _ROUTING_FRAMEWORK_H_

#include "Yggdrasil.h"

#include "data_structures/double_list.h"

#include "routing_algorithm/routing_algorithms.h"

#define ROUTING_FRAMEWORK_PROTO_ID 161
#define ROUTING_FRAMEWORK_PROTO_NAME "ROUTING FRAMEWORK"

typedef struct _routing_stats {
    unsigned long messages_transmitted;
    unsigned long messages_delivered;
    unsigned long messages_received;
    unsigned long messages_requested;
} routing_stats;

typedef struct _routing_framework_args {
	RoutingAlgorithm* algorithm;

    unsigned long max_jitter_ms;
    unsigned long period_margin_ms;

    unsigned int announce_misses;
    unsigned long min_announce_interval_ms;

    bool ignore_zero_seq;

	unsigned long seen_expiration_ms;
	unsigned long gc_interval_s;

} routing_framework_args;

proto_def* routing_framework_init(void* arg);
void* routing_framework_main_loop(main_loop_args* args);

routing_framework_args* new_routing_framework_args(RoutingAlgorithm* algorithm, unsigned long seen_expiration_ms, unsigned long gc_interval_s, unsigned long max_jitter_ms, unsigned long period_margin_ms, unsigned int announce_misses, unsigned long min_announce_interval_ms, bool ignore_zero_seq);

routing_framework_args* default_routing_framework_args();

routing_framework_args* load_routing_framework_args(const char* file_path);


void RouteMessage(unsigned char* destination_id, short protocol_id, unsigned short ttl, unsigned char* data, unsigned int size);

typedef enum {
    REQ_ROUTE_MESSAGE = 0,
	REQ_ROUTING_FRAMEWORK_STATS,
    ROUTING_REQ_TYPE_COUNT
} RoutingRequestType;

typedef enum {
	TIMER_PERIODIC_ANNOUNCE = 0,
    TIMER_SOURCE_ENTRY,
    TIMER_SEND,
    TIMER_RETRY,
	ROUTING_TIMER_TYPE_COUNT
} RoutingTimerType;

typedef enum {
	MSG_ROUTING_MESSAGE = 0,
    MSG_CONTROL_MESSAGE,
	ROUTING_MSG_TYPE_COUNT
} RoutingMessageType;

#endif /* _ROUTING_FRAMEWORK_H_ */
