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

#ifndef _DISCOVERY_DELIVERY_H_
#define _DISCOVERY_DELIVERY_H_

#include "messages.h"
#include "neighbors_table.h"

typedef struct HelloDeliverSummary_ {
    NeighborEntry* neigh;

    bool new_neighbor;
    bool updated_neighbor;
    bool rebooted;
    bool period_changed;
    bool updated_quality;
    bool updated_quality_threshold;
    bool updated_traffic;
    bool updated_traffic_threshold;
    int missed_hellos;
} HelloDeliverSummary;

typedef struct HackDeliverSummary_ {
    NeighborEntry* neigh;

    bool positive_hack;
    bool updated_neighbor;
    int missed_hacks;
    bool new_hack;
    bool repeated_yet_fresh_hack;
    bool became_bi;
    bool lost_bi;
    bool period_changed;
    bool updated_quality;
    bool updated_quality_threshold;

    bool updated_2hop_quality;
    bool updated_2hop_quality_threshold;
    bool updated_2hop_traffic;
    bool updated_2hop_traffic_threshold;
    bool became_bi_2hop;
    bool lost_bi_2hop;

    bool updated_2hop_neighbor;
    bool added_2hop_neighbor;
    bool lost_2hop_neighbor;
} HackDeliverSummary;

typedef struct NeighborTimerSummary_ {
    NeighborEntry* neigh;

    bool updated_neighbor;
    bool lost_neighbor;
    bool removed;
    bool lost_bi;
    bool updated_quality;
    bool updated_quality_threshold;
    int missed_hellos;
    int missed_hacks;

    unsigned int deleted_2hop;

    //bool updated_2hop_neighbor;
    //bool added_2hop_neighbor;
    bool lost_2hop_neighbor;
} NeighborTimerSummary;

typedef enum {
    PIGGYBACK_MSG,
    PERIODIC_MSG,
    REPLY_MSG,
    NEIGHBOR_CHANGE_MSG
} MessageType;

typedef struct NeighborChangeSummary_ {
    //NeighborEntry* neigh;

    bool new_neighbor;
    bool updated_neighbor;
    bool lost_neighbor;

    bool updated_2hop_neighbor;
    bool added_2hop_neighbor;
    bool lost_2hop_neighbor;

    //unsigned int deleted_2hop;

    bool other;

    bool removed;
    bool rebooted;
    bool lost_bi;
    bool became_bi;
    bool hello_period_changed;
    bool hack_period_changed;
    bool updated_quality;
    bool updated_quality_threshold;
} NeighborChangeSummary;

typedef struct MessageSummary_ {
    //NeighborEntry* neigh;
    HelloDeliverSummary* hello_summary;
    //HackDeliverSummary* hack_summary;
    list* hack_summaries;
} MessageSummary;

HelloDeliverSummary* newHelloDeliverSummary();

HackDeliverSummary* newHackDeliverSummary();

NeighborTimerSummary* newNeighborTimerSummary();

MessageSummary* newMessageSummary();

HelloDeliverSummary* deliverHello(void* f_state, HelloMessage* hello, WLANAddr* addr, MessageSummary* msg_summary);

HackDeliverSummary* deliverHack(void* f_state, HackMessage* hack, MessageSummary* msg_summary);

void DF_notifyGenericEvent(char* type, void* buffer, unsigned int size);

#endif /* _DISCOVERY_DELIVERY_H_ */
