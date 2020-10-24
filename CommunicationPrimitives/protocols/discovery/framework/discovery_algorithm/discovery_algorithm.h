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
#include "discovery_message/discovery_message.h"
#include "link_quality/link_quality.h"

typedef struct _DiscoveryAlgorithm DiscoveryAlgorithm;

DiscoveryAlgorithm* newDiscoveryAlgorithm(DiscoveryPattern* d_pattern, DiscoveryPeriod* d_period, LinkQuality* lq_metric, DiscoveryMessage* d_message);

void destroyDiscoveryAlgorithm(DiscoveryAlgorithm* alg);

bool DA_periodicHello(DiscoveryAlgorithm* alg);

PiggybackType DA_piggybackHellos(DiscoveryAlgorithm* alg);

bool DA_HelloNewNeighbor(DiscoveryAlgorithm* alg);

bool DA_HelloLostNeighbor(DiscoveryAlgorithm* alg);

bool DA_HelloUpdateNeighbor(DiscoveryAlgorithm* alg);

HelloSchedulerType DA_getHelloType(DiscoveryAlgorithm* alg);

PiggybackType DA_piggybackHacks(DiscoveryAlgorithm* alg);

bool DA_periodicHack(DiscoveryAlgorithm* alg);

HackReplyType DA_replyHacksToHellos(DiscoveryAlgorithm* alg);

bool DA_HackNewNeighbor(DiscoveryAlgorithm* alg);

bool DA_HackLostNeighbor(DiscoveryAlgorithm* alg);

bool DA_HackUpdateNeighbor(DiscoveryAlgorithm* alg);

HelloSchedulerType DA_getHackType(DiscoveryAlgorithm* alg);

byte DA_getHelloPeriod(DiscoveryAlgorithm* alg);

byte DA_getHackPeriod(DiscoveryAlgorithm* alg);

byte DA_computeNextHelloPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, NeighborsTable* neighbors);

byte DA_computeNextHackPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, NeighborsTable* neighbors);

double DA_computeLinkQuality(DiscoveryAlgorithm* alg, void* lq_attrs, double previous_link_quality, unsigned int received, unsigned int lost, bool init, struct timespec* current_time);

void* DA_createLinkQualityAttributes(DiscoveryAlgorithm* alg);

void DA_destroyLinkQualityAttributes(DiscoveryAlgorithm* alg, void* lq_attrs);

void DA_createDiscoveryMessage(DiscoveryAlgorithm* alg, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size);

bool DA_processDiscoveryMessage(DiscoveryAlgorithm* alg, void* f_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size);

void* DA_createMessageAttributes(DiscoveryAlgorithm* alg);

void DA_destroyMessageAttributes(DiscoveryAlgorithm* alg, void* msg_attributes);

#endif /* _DISCOVERY_ALGORITHM_H_ */