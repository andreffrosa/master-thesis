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

#ifndef _DISCOVERY_FRAMEWORK_HANDLERS_H_
#define _DISCOVERY_FRAMEWORK_HANDLERS_H_

#include "utility/my_time.h"
#include "utility/my_math.h"
#include "utility/my_misc.h"
#include "utility/byte.h"

#include "data_structures/double_list.h"

#include "internal_events.h"
#include "messages.h"
#include "delivery.h"
#include "neighbors_table.h"
#include "discovery_environment.h"

#include "framework.h"

typedef struct discovery_framework_state_ {
    discovery_framework_args* args;             // Framework's arguments
    queue_t* dispatcher_queue;                  //
    struct timespec current_time;               // Timestamp of the current time

    uuid_t myID;             		            // Current node id
    WLANAddr myAddr;                            // Current node address
    unsigned short my_seq;                      //
    NeighborsTable* neighbors;  	            // Table of neighbors
    DiscoveryEnvironment* environment;          //

    // HELLO Timer
    bool hello_timer_active;                    //
    uuid_t hello_timer_id;		                //
    struct timespec last_hello_time;            //
    struct timespec next_hello_time;            //

    // HACK Timer
    bool hack_timer_active;                     //
    uuid_t hack_timer_id;		                //
    struct timespec last_hack_time;             //
    struct timespec next_hack_time;             //

    // Neighbor Change Timer
    bool neighbor_change_timer_active;          //
    uuid_t neighbor_change_timer_id;            //
    //struct timespec set_neighbor_change_time;   //
    //NeighborChangeSummary neighbor_change_summary;
    struct timespec next_reactive_hello_time;
    struct timespec next_reactive_hack_time;

    uuid_t discovery_environment_timer_id;                    //

    discovery_stats stats;       	            // Framework's stats
} discovery_framework_state;


void DF_dispatchMessage(queue_t* dispatcher_queue, YggMessage* msg);

void DF_init(discovery_framework_state* state);

// Timers

void DF_uponHelloTimer(discovery_framework_state* state);

void DF_uponHackTimer(discovery_framework_state* state);

void DF_uponReplyTimer(discovery_framework_state* state, unsigned char* timer_payload, unsigned short timer_payload_size);

void DF_uponNeighborChangesTimer(discovery_framework_state* state);

bool DF_uponNeighborTimer(discovery_framework_state* state, unsigned char* neigh_id);

void DF_uponDiscoveryEnvironmentTimer(discovery_framework_state* state);

void scheduleNeighborTimer(discovery_framework_state* state, NeighborEntry* neigh);

void scheduleHelloTimer(discovery_framework_state* state, bool now);

void scheduleHackTimer(discovery_framework_state* state, bool now);

void scheduleReply(discovery_framework_state* state, HelloMessage* hello, HelloDeliverSummary* summary);

void scheduleNeighborChange(discovery_framework_state* state, HelloDeliverSummary* hello_summary, HackDeliverSummary* hack_summary, NeighborTimerSummary* neighbor_timer_summary, bool other);

void DF_uponNeighborChangeTimer(discovery_framework_state* state);

DiscoverySendPack* DF_triggerDiscoveryEvent(discovery_framework_state* state, DiscoveryInternalEventType event_type, void* event_args, YggMessage* msg);

DiscoverySendPack* DF_handleDiscoveryEvent(discovery_framework_state* state, DiscoveryInternalEventType event_type, void* event_args, YggMessage* msg);


// Messages

void DF_serialize(discovery_framework_state* state, HelloMessage* hello, HackMessage* hacks, byte n_hacks, bool piggybacked, byte* buffer, unsigned short* buffer_size);

void DF_deserialize(discovery_framework_state* state, byte* data, unsigned short size, bool piggybacked, HelloMessage** hello, HackMessage** hacks, byte* n_hacks);

void DF_processMessage(discovery_framework_state* state, byte* data, unsigned short size, bool piggybacked, WLANAddr* mac_addr);

void DF_sendMessage(discovery_framework_state* state, DiscoverySendPack* dsp, YggMessage* msg);

void DF_piggybackDiscovery(discovery_framework_state* state, YggMessage* msg);

HelloDeliverSummary* DF_uponHelloMessage(discovery_framework_state* state, HelloMessage* hello, WLANAddr* mac_addr);

HackDeliverSummary* DF_uponHackMessage(discovery_framework_state* state, HackMessage* hack);


// Requests

void DF_uponStatsRequest(discovery_framework_state* state, YggRequest* req);

// Auxiliary Functions

void DF_createHello(discovery_framework_state* state, HelloMessage* hello, bool request_replies);

void DF_createHack(discovery_framework_state* state, HackMessage* hack, NeighborEntry* neigh, bool single);

void DF_createHackBatch(discovery_framework_state* state, HackMessage** hacks, byte* n_hacks, NeighborsTable* neighbors);

void DF_createMessage(discovery_framework_state* state, HelloMessage* hello, HackMessage* hacks, byte n_hacks, DiscoveryInternalEventType event_type, void* event_args, YggMessage* msg, WLANAddr* addr);

void DF_notifyDiscoveryEnvironment(discovery_framework_state* state);

void DF_notifyNewNeighbor(discovery_framework_state* state, NeighborEntry* neigh);

void DF_notifyUpdateNeighbor(discovery_framework_state* state, NeighborEntry* neigh);

void DF_notifyLostNeighbor(discovery_framework_state* state, NeighborEntry* neigh);

void DF_notifyNeighborhood(discovery_framework_state* state);


//void changeAlgorithm(discovery_framework_state* state, DiscoveryAlgorithm* new_alg);

void DF_printNeighbors(discovery_framework_state* state);

void DF_printStats(discovery_framework_state* state);

void flushNeighbor(discovery_framework_state* state, NeighborEntry* neigh);


#endif /* _DISCOVERY_FRAMEWORK_HANDLERS_H_ */
