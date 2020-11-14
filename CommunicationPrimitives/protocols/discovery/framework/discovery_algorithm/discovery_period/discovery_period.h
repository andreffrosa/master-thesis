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

#ifndef _DISCOVER_PERIOD_H_
#define _DISCOVER_PERIOD_H_

#include "../common.h"

typedef struct _DiscoveryPeriod DiscoveryPeriod;

void destroyDiscoveryPeriod(DiscoveryPeriod* discovery_period);

// byte DP_getHelloPeriod(DiscoveryPeriod* discovery_period);

// byte DP_getHackPeriod(DiscoveryPeriod* discovery_period);

byte DP_getHelloAnnouncePeriod(DiscoveryPeriod* discovery_period);

byte DP_getHelloTransmitPeriod(DiscoveryPeriod* discovery_period, struct timespec* current_time);

byte DP_getHackAnnouncePeriod(DiscoveryPeriod* discovery_period);

byte DP_getHackTransmitPeriod(DiscoveryPeriod* discovery_period, struct timespec* current_time);

byte DP_computeNextHelloPeriod(DiscoveryPeriod* discovery_period, unsigned long elapsed_time_ms, unsigned int transition_period_n, NeighborsTable* neighbors, struct timespec* current_time);

byte DP_computeNextHackPeriod(DiscoveryPeriod* discovery_period, unsigned long elapsed_time_ms, unsigned int transition_period_n, NeighborsTable* neighbors, struct timespec* current_time);

DiscoveryPeriod* StaticDiscoveryPeriod(byte initial_hello_period_s, byte initial_hack_period_s);

#endif /* _DISCOVER_PERIOD_H_ */
