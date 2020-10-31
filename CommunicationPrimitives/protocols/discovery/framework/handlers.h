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

#include "messages.h"

#include "delivery.h"

#include "data_structures/double_list.h"

#include "neighbors_table.h"

#include "framework.h"

typedef struct discovery_framework_state_ {
    discovery_framework_args* args;             // Framework's arguments
    queue_t* dispatcher_queue;                  //
    struct timespec current_time;               // Timestamp of the current time

    uuid_t myID;             		            // Current node id
    WLANAddr myAddr;                            // Current node address
    unsigned short my_seq;                      //
    NeighborsTable* neighbors;  	            // Table of neighbors

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
    struct timespec set_neighbor_change_time;   //
    //struct timespec next_neighbor_change_time;  //
    NeighborChangeSummary neighbor_change_summary;

    uuid_t windows_timer_id;                    //
    discovery_stats stats;       	            // Framework's stats

    /*
    //uuid_t announce_timer_id;		        // timer id
	               //
    struct timespec last_heartbeat_date;    // Timestamp of the last heartbeat sent (piggybacked or not)
    struct timespec next_heartbeat_date;    // Timestamp of the next heartbeat to send
    bool hb_timer_is_expired;
    bool announce_scheduled;                //
    list* triggered_events;                 //

    YggMessage cached_announce;

    uuid_t gc_timer_id;			 		    // Garbage Collector timer id
    bool gc_active;                         //

    uuid_t windows_timer_id;
    Window* out_traffic, *stability;

    unsigned long heartbeat_period_s;
*/

} discovery_framework_state;


void DF_dispatchMessage(queue_t* dispatcher_queue, YggMessage* msg);

void DF_init(discovery_framework_state* state);

// Timers

void DF_uponHelloTimer(discovery_framework_state* state, bool periodic, bool send_hack);

void DF_uponHackTimer(discovery_framework_state* state, bool periodic);

void DF_uponReplyTimer(discovery_framework_state* state, unsigned char* timer_payload, unsigned short timer_payload_size);

void DF_uponNeighborChangesTimer(discovery_framework_state* state);

void DF_uponWindowsTimer(discovery_framework_state* state);

bool DF_uponNeighborTimer(discovery_framework_state* state, NeighborEntry* neigh);

void scheduleHelloTimer(discovery_framework_state* state, bool now);

void scheduleHackTimer(discovery_framework_state* state, bool now);

void scheduleReply(discovery_framework_state* state, HelloMessage* hello);

void scheduleNeighborChange(discovery_framework_state* state, HelloDeliverSummary* hello_summary, HackDeliverSummary* hack_summary, NeighborTimerSummary* neighbor_timer_summary, bool other);

void DF_uponNeighborChangeTimer(discovery_framework_state* state);

// Messages

void DF_serialize(discovery_framework_state* state, HelloMessage* hello, HackMessage* hacks, byte n_hacks, bool piggybacked, byte* buffer, unsigned short* buffer_size);

void DF_deserialize(discovery_framework_state* state, byte* data, unsigned short size, bool piggybacked, HelloMessage** hello, HackMessage** hacks, byte* n_hacks);

void DF_processMessage(discovery_framework_state* state, byte* data, unsigned short size, bool piggybacked, WLANAddr* mac_addr);

bool DF_sendMessage(discovery_framework_state* state, HelloMessage* hello, HackMessage* hacks, byte n_hacks, WLANAddr* addr, MessageType msg_type, void* aux_info);

void DF_piggybackDiscovery(discovery_framework_state* state, YggMessage* msg);

HelloDeliverSummary* DF_uponHelloMessage(discovery_framework_state* state, HelloMessage* hello, WLANAddr* mac_addr);

HackDeliverSummary* DF_uponHackMessage(discovery_framework_state* state, HackMessage* hack);


// Requests

void DF_uponStatsRequest(discovery_framework_state* state, YggRequest* req);

// Auxiliary Functions

void DF_createHello(discovery_framework_state* state, HelloMessage* hello, bool request_replies);

void DF_createHack(discovery_framework_state* state, HackMessage* hack, NeighborEntry* neigh, bool single);

void DF_createHackBatch(discovery_framework_state* state, HackMessage** hacks, byte* n_hacks, NeighborsTable* neighbors);

void DF_notifyNewNeighbor(discovery_framework_state* state, NeighborEntry* neigh);

void DF_notifyUpdateNeighbor(discovery_framework_state* state, NeighborEntry* neigh);

void DF_notifyLostNeighbor(discovery_framework_state* state, NeighborEntry* neigh);

void changeAlgorithm(discovery_framework_state* state, DiscoveryAlgorithm* new_alg);

void DF_printNeighbors(discovery_framework_state* state);

void DF_printStats(discovery_framework_state* state);

void flushNeighbor(discovery_framework_state* state, NeighborEntry* neigh);


/*
void scheduleHeartbeat(discovery_framework_state* state, bool now, unsigned long jitter);
unsigned long scheduleAnnounce(discovery_framework_state* state, bool force);

void changeHeartbeatPeriod(discovery_framework_state* state, unsigned long new_heartbeat_period_s);

void uponHeartbeatTimer(discovery_framework_state* state);
void uponAnnounceTimer(discovery_framework_state* state);
void uponWindowsTimer(discovery_framework_state* state);
void DF_uponGarbageCollectorTimer(discovery_framework_state* state);
void DF_uponStatsRequest(discovery_framework_state* state, YggRequest* req);

void piggybackHeartbeat(discovery_framework_state* state, YggMessage* msg, bool inc);

void processHeartbeat(discovery_framework_state* state, HeartbeatHeader* hb, YggMessage* announce, bool has_announce);

void DF_dispatchMessage(queue_t* dispatcher_queue, YggMessage* msg);

bool triggerDiscoveryEvent(discovery_framework_state* state, YggEvent* ev, bool private, bool force);
*/



/*void uponBroadcastRequest(discovery_framework_state* state, YggRequest* req);
void uponNewMessage(discovery_framework_state* state, YggMessage* msg);
void uponTimeout(discovery_framework_state* state, YggTimer* timer);

void init(discovery_framework_state* state);
void ComputeRetransmissionDelay(discovery_framework_state* state, PendingMessage* p_msg, bool isCopy);
void changePhase(discovery_framework_state* state, PendingMessage* p_msg);
void DeliverMessage(discovery_framework_state* state, YggMessage* toDeliver);
void RetransmitMessage(discovery_framework_state* state, PendingMessage* p_msg, unsigned short ttl);
void uponBroadcastRequest(discovery_framework_state* state, YggRequest* req);
void uponNewMessage(discovery_framework_state* state, YggMessage* msg);
void uponTimeout(discovery_framework_state* state, YggTimer* timer);
void serializeHeader(discovery_framework_state* state, PendingMessage* p_msg, bcast_header* header, void** context_header, unsigned short ttl);
void serializeMessage(discovery_framework_state* state, YggMessage* m, PendingMessage* p_msg, unsigned short ttl);
void deserializeMessage(YggMessage* m, bcast_header* header, void** context_header, YggMessage* toDeliver);
void runGarbageCollector(discovery_framework_state* state);
void uponStatsRequest(discovery_framework_state* state, YggRequest* req);*/

#endif /* _DISCOVERY_FRAMEWORK_HANDLERS_H_ */
