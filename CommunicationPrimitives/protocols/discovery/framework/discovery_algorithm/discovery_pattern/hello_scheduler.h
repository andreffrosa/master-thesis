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

#ifndef _DISCOVERY_HELLO_SCHEDULER_H_
#define _DISCOVERY_HELLO_SCHEDULER_H_

#include "../common.h"

#include "discovery_pattern_common.h"

typedef struct _HelloScheduler HelloScheduler;

typedef enum {
    NO_HELLO,
    PIGGYBACK_HELLO,
    PERIODIC_HELLO,
    HYBRID_HELLO
} HelloSchedulerType;

HelloScheduler* newHelloScheduler(HelloSchedulerType hello_type, PiggybackFilter* piggyback_filter, PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_update_neighbor, bool react_to_lost_neighbor, bool react_to_context_updates);

void destroyHelloScheduler(HelloScheduler* hello_sh);

HelloScheduler* NoHELLO();

HelloScheduler* PiggybackHELLO(PiggybackFilter* piggyback_filter);

HelloScheduler* PeriodicHELLO(PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_update_neighbor, bool react_to_lost_neighbor, bool react_to_context_updates);

HelloScheduler* HybridHELLO(PiggybackFilter* piggyback_filter, bool react_to_new_neighbor, bool react_to_update_neighbor, bool react_to_lost_neighbor, bool react_to_context_updates);

void HELLO_update(HelloScheduler* hello_sh, HelloSchedulerType hello_type, PiggybackFilter* piggyback_filter, PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_update_neighbor, bool react_to_lost_neighbor, bool react_to_context_updates);

PeriodicType HELLO_periodicType(HelloScheduler* hello_sh);

PiggybackType HELLO_evalPiggybackFilter(HelloScheduler* hello_sh, YggMessage* msg, void* extra_args);

bool HELLO_newNeighbor(HelloScheduler* hello_sh);

bool HELLO_lostNeighbor(HelloScheduler* hello_sh);

bool HELLO_updateNeighbor(HelloScheduler* hello_sh);

bool HELLO_updateContext(HelloScheduler* hello_sh);

HelloSchedulerType HELLO_getType(HelloScheduler* hello_sh);

#endif /* _DISCOVERY_HELLO_SCHEDULER_H_ */
