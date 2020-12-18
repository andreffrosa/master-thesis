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

#ifndef _DISCOVERY_HACK_SCHEDULER_H_
#define _DISCOVERY_HACK_SCHEDULER_H_

#include "../common.h"

#include "discovery_pattern_common.h"

typedef struct _HackScheduler HackScheduler;

typedef enum {
    NO_HACK,
    PIGGYBACK_HACK,
    PERIODIC_HACK,
    HYBRID_HACK,
    REPLY_HACK
} HackSchedulerType;

typedef enum {
    NO_HACK_REPLY,
    BROADCAST_HACK_REPLY,
    //BROADCAST_HELLO_HACK_REPLY,
    UNICAST_HACK_REPLY
} HackReplyType;

HackScheduler* newHackScheduler(HackSchedulerType hack_type, PiggybackFilter* piggyback_filter, PeriodicType periodic_type, HackReplyType reply_to_hellos, bool react_to_new_neighbor, bool react_to_update_neighbor, bool react_to_lost_neighbor, bool react_to_context_updates);

void destroyHackScheduler(HackScheduler* hack_sh);

HackScheduler* NoHACK();

HackScheduler* PiggybackHACK(PiggybackFilter* piggyback_filter);

HackScheduler* PeriodicHACK(PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_update_neighbor, bool react_to_lost_neighbor, bool react_to_context_updates);

HackScheduler* HybridHACK(PiggybackFilter* piggyback_filter, bool react_to_new_neighbor, bool react_to_update_neighbor, bool react_to_lost_neighbor, bool react_to_context_updates);

HackScheduler* ReplyHACK(HackReplyType reply_type);

void HACK_update(HackScheduler* hack_sh, HackSchedulerType hack_type, PiggybackFilter* piggyback_filter, PeriodicType periodic_type, HackReplyType reply_to_hellos, bool react_to_new_neighbor, bool react_to_update_neighbor, bool react_to_lost_neighbor, bool react_to_context_updates);

PeriodicType HACK_periodicType(HackScheduler* hack_sh);

PiggybackType HACK_evalPiggybackFilter(HackScheduler* hack_sh, YggMessage* msg, void* extra_args);

HackReplyType HACK_replyToHellos(HackScheduler* hack_sh);

bool HACK_newNeighbor(HackScheduler* hack_sh);

bool HACK_lostNeighbor(HackScheduler* hack_sh);

bool HACK_updateNeighbor(HackScheduler* hack_sh);

bool HACK_updateContext(HackScheduler* hack_sh);

HackSchedulerType HACK_getType(HackScheduler* hack_sh);

#endif /* _DISCOVERY_HACK_SCHEDULER_H_ */
