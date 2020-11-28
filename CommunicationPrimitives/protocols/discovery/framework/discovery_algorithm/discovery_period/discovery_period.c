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
    discovery_period->old_hello_period_s = 0;
    discovery_period->old_hack_period_s = 0;
    discovery_period->compute_next_period = compute_next_period;
    copy_timespec(&discovery_period->transition_time, &zero_timespec);

    return discovery_period;
}

void destroyDiscoveryPeriod(DiscoveryPeriod* discovery_period) {
    if(discovery_period) {
        free(discovery_period);
    }
}

/*
byte DP_getHelloPeriod(DiscoveryPeriod* discovery_period) {
    assert(discovery_period);
    return discovery_period->hello_period_s;
}

byte DP_getHackPeriod(DiscoveryPeriod* discovery_period) {
    assert(discovery_period);
    return discovery_period->hack_period_s;
}
*/



byte DP_getHelloAnnouncePeriod(DiscoveryPeriod* discovery_period) {
    assert(discovery_period);
    return discovery_period->hello_period_s;
}

byte DP_getHelloTransmitPeriod(DiscoveryPeriod* discovery_period, struct timespec* current_time) {
    assert(discovery_period);

    if( compare_timespec(&discovery_period->transition_time, current_time) > 0 ) {
        return discovery_period->old_hello_period_s;
    } else {
        return discovery_period->hello_period_s;
    }
}

byte DP_getHackAnnouncePeriod(DiscoveryPeriod* discovery_period) {
    assert(discovery_period);
    return discovery_period->hack_period_s;
}

byte DP_getHackTransmitPeriod(DiscoveryPeriod* discovery_period, struct timespec* current_time) {
    assert(discovery_period);

    if( compare_timespec(&discovery_period->transition_time, current_time) > 0 ) {
        return discovery_period->old_hack_period_s;
    } else {
        return discovery_period->hack_period_s;
    }
}

byte DP_computeNextHelloPeriod(DiscoveryPeriod* discovery_period, unsigned long elapsed_time_ms, unsigned int transition_period_n, NeighborsTable* neighbors, struct timespec* current_time) {
    assert(discovery_period);

    discovery_period->old_hello_period_s = discovery_period->hello_period_s;

    discovery_period->hello_period_s = discovery_period->compute_next_period("hello", discovery_period->hello_period_s, elapsed_time_ms, neighbors);

    if( discovery_period->hello_period_s > discovery_period->old_hello_period_s ) {
        struct timespec t;
        milli_to_timespec(&t, transition_period_n * discovery_period->old_hello_period_s);
        add_timespec(&discovery_period->transition_time, &t, current_time);
    }

    return discovery_period->hello_period_s;
}

byte DP_computeNextHackPeriod(DiscoveryPeriod* discovery_period, unsigned long elapsed_time_ms, unsigned int transition_period_n, NeighborsTable* neighbors, struct timespec* current_time) {
    assert(discovery_period);

    discovery_period->old_hack_period_s = discovery_period->hack_period_s;

    discovery_period->hack_period_s = discovery_period->compute_next_period("hack", discovery_period->hack_period_s, elapsed_time_ms, neighbors);

    if( discovery_period->hack_period_s > discovery_period->old_hack_period_s ) {
        struct timespec t;
        milli_to_timespec(&t, transition_period_n * discovery_period->old_hack_period_s);
        add_timespec(&discovery_period->transition_time, &t, current_time);
    }

    return discovery_period->hack_period_s;
}
