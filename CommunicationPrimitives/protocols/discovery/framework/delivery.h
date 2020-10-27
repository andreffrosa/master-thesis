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

typedef struct HelloDeliverSummary_ {
    bool new_neighbor;
    bool updated_neighbor; // TODO: verificar se está corretamente a ser set
    bool rebooted;
    bool period_changed;
    bool updated_quality;
    bool updated_quality_threshold;
    bool updated_traffic;
    bool updated_traffic_threshold;
    int lost_messages;
} HelloDeliverSummary;

typedef struct HackDeliverSummary_ {
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

    bool updated_two_hop_neighbor;
    bool added_two_hop_neighbor;
    bool removed_two_hop_neighbor;
} HackDeliverSummary;

typedef struct ScheduleSummary_ {
    bool piggyback;
    bool periodic;
    bool reply;
    bool neighbor_change;
} ScheduleSummary;

HelloDeliverSummary* newHelloDeliverSummary();

HackDeliverSummary* newHackDeliverSummary();

HelloDeliverSummary* deliverHello(void* f_state, HelloMessage* hello, WLANAddr* addr);

HackDeliverSummary* deliverHack(void* f_state, HackMessage* hack);

#endif /* _DISCOVERY_DELIVERY_H_ */
