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

#ifndef _DISCOVERY_PIGGYBACK_FILTER_H_
#define _DISCOVERY_PIGGYBACK_FILTER_H_

#include "../common.h"

/*
typedef enum {
    NO_PIGGYBACK,
    PIGGYBACK_ON_UNICAST_TRAFFIC,
    PIGGYBACK_ON_BROADCAST_TRAFFIC,
    PIGGYBACK_ON_DISCOVERY_TRAFFIC,
    PIGGYBACK_ON_ALL_TRAFFIC,
} PiggybackType;
*/

typedef enum {
    NO_PIGGYBACK,        // no piggyback
    UNICAST_PIGGYBACK,   // the message remains unicast
    BROADCAST_PIGGYBACK, // convert unicast messages to broadcast if necessary
} PiggybackType;

typedef PiggybackType (*piggyback_filter)(ModuleState*, YggMessage*, void*);

typedef void (*piggyback_filter_destroy)(ModuleState*);

typedef struct PiggybackFilter_ {
    ModuleState state;
    piggyback_filter filter;
    piggyback_filter_destroy destroy;
} PiggybackFilter;

PiggybackFilter* newPiggybackFilter(void* args, void* vars, piggyback_filter filter, piggyback_filter_destroy destroy);

void destroyPiggybackFilter(PiggybackFilter* pf);

PiggybackType evalPiggybackFilter(PiggybackFilter* pf, YggMessage* msg, void* extra_args);

/////////////////////////////////////////////

PiggybackFilter* NoPiggyback();

PiggybackFilter* PiggybackOnUnicast();

PiggybackFilter* PiggybackOnBroadcast();

PiggybackFilter* PiggybackOnDiscovery();

PiggybackFilter* PiggybackOnAll(bool convert_to_broadcast);

PiggybackFilter* PiggybackOnNewNeighbor();


#endif /* _DISCOVERY_PIGGYBACK_FILTER_H_ */
