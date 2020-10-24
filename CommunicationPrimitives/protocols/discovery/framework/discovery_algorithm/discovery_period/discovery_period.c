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

#include <assert.h>

DiscoveryPeriod* newDiscoveryPeriod(byte initial_hello_period_s, byte initial_hack_period_s, next_period_function compute_next_period) {
    DiscoveryPeriod* discovery_period = malloc(sizeof(DiscoveryPeriod));

    discovery_period->hello_period_s = initial_hello_period_s;
    discovery_period->hack_period_s = initial_hack_period_s;
    discovery_period->compute_next_period = compute_next_period;

    return discovery_period;
}

void destroyDiscoveryPeriod(DiscoveryPeriod* discovery_period) {
    if(discovery_period) {
        free(discovery_period);
    }
}

byte DP_getHelloPeriod(DiscoveryPeriod* discovery_period) {
    assert(discovery_period);
    return discovery_period->hello_period_s;
}

byte DP_getHackPeriod(DiscoveryPeriod* discovery_period) {
    assert(discovery_period);
    return discovery_period->hack_period_s;
}

byte DP_computeNextHelloPeriod(DiscoveryPeriod* discovery_period, unsigned long elapsed_time_ms, NeighborsTable* neighbors) {
    assert(discovery_period);
    byte next_hello_period = discovery_period->compute_next_period("hello", discovery_period->hello_period_s, elapsed_time_ms, neighbors);
    discovery_period->hello_period_s = next_hello_period;
    return next_hello_period;
}

byte DP_computeNextHackPeriod(DiscoveryPeriod* discovery_period, unsigned long elapsed_time_ms, NeighborsTable* neighbors) {
    assert(discovery_period);
    byte next_hack_period = discovery_period->compute_next_period("hack", discovery_period->hack_period_s, elapsed_time_ms, neighbors);
    discovery_period->hack_period_s = next_hack_period;
    return next_hack_period;
}
