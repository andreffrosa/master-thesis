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

#ifndef _ROUTING_FRAMEWORK_HANDLERS_H_
#define _ROUTING_FRAMEWORK_HANDLERS_H_

#include "data_structures/double_list.h"

#include "seen_messages.h"
#include "routing_header.h"
#include "routing_algorithm/routing_algorithms.h"

#include "routing_table.h"
#include "routing_neighbors.h"
#include "source_set.h"

#include "protocols/discovery/framework/framework.h"

#include "update.h"

#include "framework.h"

typedef struct routing_framework_state_ {
    routing_framework_args* args;             // Framework's arguments
    struct timespec current_time;               // Timestamp of the current time

    uuid_t myID;             		            // Current node id
    WLANAddr myAddr;                            // Current node address
    unsigned short my_seq;                      //

    RoutingTable* routing_table;     // Routing Table
    RoutingNeighbors* neighbors;
    SourceSet* source_set;

	SeenMessages* seen_msgs;  	    // List of seen messages so far
	uuid_t gc_id;			 		// Garbage Collector timer id

    uuid_t announce_timer_id;       // Periodic Announce timer id
    struct timespec last_announce_time;
    bool announce_timer_active;

	routing_stats stats;       		// Framework's stats

} routing_framework_state;

// Protocol Handlers

void RF_init(routing_framework_state* state);

void scheduleAnnounceTimer(routing_framework_state* state, bool now);
void RF_uponAnnounceTimer(routing_framework_state* state);

void RF_uponDiscoveryEvent(routing_framework_state* state, YggEvent* ev);

void RF_triggerEvent(routing_framework_state* state, RoutingEventType event_type, void* event_args);

void RF_uponNewControlMessage(routing_framework_state* state, YggMessage* msg);

void RF_runGarbageCollector(routing_framework_state* state);

void RF_uponStatsRequest(routing_framework_state* state, YggRequest* req);


// void RF_disseminateAnnounce(routing_framework_state* state);

#endif /* _ROUTING_FRAMEWORK_HANDLERS_H_ */
