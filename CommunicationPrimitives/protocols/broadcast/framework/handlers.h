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

#ifndef _BCAST_FRAMEWORK_HANDLERS_H_
#define _BCAST_FRAMEWORK_HANDLERS_H_

#include "data_structures/double_list.h"

#include "pending_messages/pending_messages.h"
#include "bcast_header.h"
//#include "broadcast_algorithm/bcast_algorithms.h"

#include "framework.h"

typedef struct broadcast_framework_state_ {
	PendingMessages* seen_msgs;  	// List of seen messages so far
	uuid_t gc_id;			 		// Garbage Collector timer id
	uuid_t myID;             		// Current process id
	broadcast_framework_args* args; // Framework's arguments
	broadcast_stats stats;       		// Framework's stats
	struct timespec current_time;   // Timestamp of the current time
} broadcast_framework_state;

// Protocol Handlers
void uponBroadcastRequest(broadcast_framework_state* state, YggRequest* req);
void uponNewMessage(broadcast_framework_state* state, YggMessage* msg);
void uponTimeout(broadcast_framework_state* state, YggTimer* timer);

void init(broadcast_framework_state* state);
void ComputeRetransmissionDelay(broadcast_framework_state* state, PendingMessage* p_msg, bool isCopy);
void changePhase(broadcast_framework_state* state, PendingMessage* p_msg);
void DeliverMessage(broadcast_framework_state* state, YggMessage* toDeliver);
void RetransmitMessage(broadcast_framework_state* state, PendingMessage* p_msg, unsigned short ttl);
void uponBroadcastRequest(broadcast_framework_state* state, YggRequest* req);
void uponNewMessage(broadcast_framework_state* state, YggMessage* msg);
void uponTimeout(broadcast_framework_state* state, YggTimer* timer);
void serializeHeader(broadcast_framework_state* state, PendingMessage* p_msg, bcast_header* header, void** context_header, unsigned short ttl);
void serializeMessage(broadcast_framework_state* state, YggMessage* m, PendingMessage* p_msg, unsigned short ttl);
void deserializeMessage(YggMessage* m, bcast_header* header, void** context_header, YggMessage* toDeliver);
void runGarbageCollector(broadcast_framework_state* state);
void uponStatsRequest(broadcast_framework_state* state, YggRequest* req);

#endif /* _BCAST_FRAMEWORK_HANDLERS_H_ */
