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

#ifndef _DISCOVERY_HELLO_SCHEDULER_PRIVATE_H_
#define _DISCOVERY_HELLO_SCHEDULER_PRIVATE_H_

#include "hello_scheduler.h"

#include "Yggdrasil.h"

typedef struct _HelloScheduler {
    HelloSchedulerType type;
    PiggybackFilter* piggyback_filter;
    PeriodicType periodic_type;
    bool react_to_new_neighbor;
    bool react_to_lost_neighbor;
    bool react_to_update_neighbor;
    bool react_to_new_2hop_neighbor;
    bool react_to_lost_2hop_neighbor;
    bool react_to_update_2hop_neighbor;
} HelloScheduler;

#endif /* _DISCOVERY_HELLO_SCHEDULER_PRIVATE_H_ */
