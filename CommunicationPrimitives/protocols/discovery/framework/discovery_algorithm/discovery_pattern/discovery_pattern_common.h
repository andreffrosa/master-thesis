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

#ifndef _DISCOVERY_PATTERN_COMMON_H_
#define _DISCOVERY_PATTERN_COMMON_H_

typedef enum {
    NO_PIGGYBACK,
    PIGGYBACK_ON_UNICAST_TRAFFIC,
    PIGGYBACK_ON_BROADCAST_TRAFFIC,
    PIGGYBACK_ON_DISCOVERY_TRAFFIC,
    PIGGYBACK_ON_ALL_TRAFFIC,
} PiggybackType;

typedef enum {
    NO_PERIODIC,
    STATIC_PERIODIC,
    RESET_PERIODIC,
} PeriodicType;

#endif /* _DISCOVERY_PATTERN_COMMON_H_ */
