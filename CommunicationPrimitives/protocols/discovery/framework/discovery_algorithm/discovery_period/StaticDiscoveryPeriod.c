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

#include "discovery_period_private.h"

static byte static_discovery_next_period(const char* type, byte previous_period_s, unsigned long elapsed_time_ms, NeighborsTable* neighbors) {
    return previous_period_s;
}

DiscoveryPeriod* StaticDiscoveryPeriod(byte initial_hello_period_s, byte initial_hack_period_s) {
    return newDiscoveryPeriod(initial_hello_period_s, initial_hack_period_s, &static_discovery_next_period);
}
