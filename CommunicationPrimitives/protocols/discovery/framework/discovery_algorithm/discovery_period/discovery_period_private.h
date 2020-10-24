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

#ifndef _DISCOVER_PERIOD_PRIVATE_H_
#define _DISCOVER_PERIOD_PRIVATE_H_

#include "discovery_period.h"

typedef byte (*next_period_function)(const char* type, byte previous_period_s, unsigned long elapsed_time_ms, NeighborsTable* neighbors);

typedef struct _DiscoveryPeriod {
    byte hello_period_s;
    byte hack_period_s;
    next_period_function compute_next_period;
} DiscoveryPeriod;

DiscoveryPeriod* newDiscoveryPeriod(byte initial_hello_period_s, byte initial_hack_period_s, next_period_function compute_next_period);

#endif /* _DISCOVER_PERIOD_PRIVATE_H_ */
