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

HackScheduler* newHackScheduler(HackSchedulerType hack_type, PiggybackType piggyback_type, bool periodic, HackReplyType reply_to_hellos, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor);

void destroyHackScheduler(HackScheduler* hack_sh);

HackScheduler* NoHACK();

HackScheduler* PiggybackHACK(PiggybackType piggyback_type);

HackScheduler* PeriodicHACK(bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor);

HackScheduler* HybridHACK(PiggybackType piggyback_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor);

HackScheduler* ReplyHACK(HackReplyType reply_type);

void HACK_update(HackScheduler* hack_sh, HackSchedulerType hack_type, PiggybackType piggyback_type, bool periodic, HackReplyType reply_to_hellos, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor);

bool HACK_isPeriodic(HackScheduler* hack_sh);

PiggybackType HACK_piggybackType(HackScheduler* hack_sh);

HackReplyType HACK_replyToHellos(HackScheduler* hack_sh);

bool HACK_newNeighbor(HackScheduler* hack_sh);

bool HACK_lostNeighbor(HackScheduler* hack_sh);

bool HACK_updateNeighbor(HackScheduler* hack_sh);

bool HACK_new2HopNeighbor(HackScheduler* hack_sh);

bool HACK_lost2HopNeighbor(HackScheduler* hack_sh);

bool HACK_update2HopNeighbor(HackScheduler* hack_sh);

HackSchedulerType HACK_getType(HackScheduler* hack_sh);

#endif /* _DISCOVERY_HACK_SCHEDULER_H_ */
