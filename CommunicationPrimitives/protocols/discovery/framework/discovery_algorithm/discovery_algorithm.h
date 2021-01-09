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

#ifndef _DISCOVERY_ALGORITHM_H_
#define _DISCOVERY_ALGORITHM_H_

#include <stdarg.h>

#include "common.h"

#include "data_structures/hash_table.h"

#include "discovery_pattern/discovery_pattern.h"
#include "discovery_period/discovery_period.h"
#include "discovery_context/discovery_context.h"
#include "link_quality/link_quality.h"
#include "link_admission/link_admission.h"

typedef struct _DiscoveryAlgorithm DiscoveryAlgorithm;

DiscoveryAlgorithm* newDiscoveryAlgorithm(DiscoveryPattern* d_pattern, DiscoveryPeriod* d_period, LinkQuality* lq_metric, LinkAdmission* la_policy, DiscoveryContext* d_context);

void destroyDiscoveryAlgorithm(DiscoveryAlgorithm* alg);

void DA_setDiscoveryPattern(DiscoveryAlgorithm* alg, DiscoveryPattern* new_d_pattern);

void DA_setDiscoveryPeriod(DiscoveryAlgorithm* alg, DiscoveryPeriod* new_d_period);

void DA_setLinkQuality(DiscoveryAlgorithm* alg, LinkQuality* new_lq_metric);

void DA_setLinkAdmission(DiscoveryAlgorithm* alg, LinkAdmission* new_la_policy);

void DA_setDiscoveryContext(DiscoveryAlgorithm* alg, DiscoveryContext* new_d_context);

PeriodicType DA_periodicHello(DiscoveryAlgorithm* alg);

// PiggybackType DA_piggybackHellos(DiscoveryAlgorithm* alg);

bool DA_HelloNewNeighbor(DiscoveryAlgorithm* alg);

bool DA_HelloLostNeighbor(DiscoveryAlgorithm* alg);

bool DA_HelloUpdateNeighbor(DiscoveryAlgorithm* alg);

bool DA_HelloContextUpdate(DiscoveryAlgorithm* alg);

HelloSchedulerType DA_getHelloType(DiscoveryAlgorithm* alg);

// PiggybackType DA_piggybackHacks(DiscoveryAlgorithm* alg);

PeriodicType DA_periodicHack(DiscoveryAlgorithm* alg);

HackReplyType DA_replyHacksToHellos(DiscoveryAlgorithm* alg);

bool DA_HackNewNeighbor(DiscoveryAlgorithm* alg);

bool DA_HackLostNeighbor(DiscoveryAlgorithm* alg);

bool DA_HackUpdateNeighbor(DiscoveryAlgorithm* alg);

bool DA_HackContextUpdate(DiscoveryAlgorithm* alg);

HelloSchedulerType DA_getHackType(DiscoveryAlgorithm* alg);

DiscoveryInternalEventResult* DA_triggerEvent(DiscoveryAlgorithm* alg, DiscoveryInternalEventType event_type, void* event_args, NeighborsTable* neighbors, YggMessage* msg);

/*
byte DA_getHelloPeriod(DiscoveryAlgorithm* alg);

byte DA_getHackPeriod(DiscoveryAlgorithm* alg);

byte DA_computeNextHelloPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, NeighborsTable* neighbors);

byte DA_computeNextHackPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, NeighborsTable* neighbors);
*/

byte DA_getHelloAnnouncePeriod(DiscoveryAlgorithm* alg);

byte DA_getHelloTransmitPeriod(DiscoveryAlgorithm* alg, struct timespec* current_time);

byte DA_getHackAnnouncePeriod(DiscoveryAlgorithm* alg);

byte DA_getHackTransmitPeriod(DiscoveryAlgorithm* alg, struct timespec* current_time);

byte DA_computeNextHelloPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, unsigned int transition_period_n, NeighborsTable* neighbors, struct timespec* current_time);

byte DA_computeNextHackPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, unsigned int transition_period_n, NeighborsTable* neighbors, struct timespec* current_time);

double DA_computeLinkQuality(DiscoveryAlgorithm* alg, void* lq_attrs, double previous_link_quality, unsigned int received, unsigned int lost, bool init, struct timespec* current_time);

void* DA_createLinkQualityAttributes(DiscoveryAlgorithm* alg);

void DA_destroyLinkQualityAttributes(DiscoveryAlgorithm* alg, void* lq_attrs);

bool DA_evalLinkAdmission(DiscoveryAlgorithm* alg, NeighborEntry* neigh, struct timespec* current_time);

void DA_createMessage(DiscoveryAlgorithm* alg, unsigned char* myID, NeighborsTable* neighbors, DiscoveryInternalEventType event_type, void* event_args, struct timespec* current_time, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size);

bool DA_processMessage(DiscoveryAlgorithm* alg, void* f_state, unsigned char* myID, NeighborsTable* neighbors, struct timespec* current_time, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size, MessageSummary* msg_summary);

bool DA_updateContext(DiscoveryAlgorithm* alg, void* f_state, unsigned char* myID, NeighborEntry* neighbor, NeighborsTable* neighbors, struct timespec* current_time, NeighborTimerSummary* summary);

void* DA_createContextAttributes(DiscoveryAlgorithm* alg);

void DA_destroyContextAttributes(DiscoveryAlgorithm* alg, void* context_attributes);

#endif /* _DISCOVERY_ALGORITHM_H_ */
