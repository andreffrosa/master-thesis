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

#ifndef _DISCOVERY_PATTERN_H_
#define _DISCOVERY_PATTERN_H_

#include "../common.h"

#include "discovery_pattern_common.h"

#include "hello_scheduler.h"
#include "hack_scheduler.h"

typedef struct _DiscoveryPattern DiscoveryPattern;

DiscoveryInternalEventResult* DP_triggerEvent(DiscoveryPattern* dp, DiscoveryInternalEventType event_type, void* event_args, NeighborsTable* neighbors, YggMessage* msg);

void destroyDiscoveryPattern(DiscoveryPattern* dp);

/*
 * No Hellos nor Hacks are sent, neither piggybacked or with explicit messages. Hellos form other nodes are ignored. Used when a node should stop discovering and being discoverd.
 */
DiscoveryPattern* NoDiscovery();

/*
 * Hellos and Hacks are both piggybacked on regular traffic. No extra messages of any sort,
 * i.e. no non-piggybacked hellos nor hacks.
 */
DiscoveryPattern* PassiveDiscovery(PiggybackFilter* hello_piggyback_filter, PiggybackFilter* hack_piggyback_filter);

/*
 * Hellos are piggybacked on regular traffic and are (1-hop) broadcasted if no
 * traffic was sent for some period. There is no hacks.
 * @Param hello_piggyback_filter : hello piggyback type.
 * @Param react_to_new_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 */
DiscoveryPattern* HybridHelloDiscovery(PiggybackFilter* hello_piggyback_filter, bool react_to_new_neighbor);

/*
 * Hellos are periodically (1-hop) broadcasted. If there are hacks, they are
 * piggyback on the hellos.
 * @Param periodic_type : specifies if the hello timer shoul be reset on each early hello or not
 * @Param react_to_new_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 */
DiscoveryPattern* PeriodicHelloDiscovery(PeriodicType periodic_type, bool react_to_new_neighbor);

/*
 * Hellos are periodically (1-hop) broadcasted. If there are hacks, they are
 * piggybacked on the hellos.
 * @Param periodic_type : specifies if the hello timer shoul be reset on each early hello or not
 * @Param react_to_new_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_lost_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_update_neighbor : specifies if an hello should be sent when
 * there is any change in some neighbor.
 */
DiscoveryPattern* PeriodicJointDiscovery(PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor);

/*
 * Both Hellos and Hacks are periodically (1-hop) broadcasted, however, in
 separate messages, usually with different periodicity.
 * @Param periodic_type : specifies if the hello and hack timers should be reset on each early hello or hack, respectively, or not
 * @Param react_to_new_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_lost_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_update_neighbor : specifies if an hello should be sent when
 * there is any change in some neighbor.
 */
DiscoveryPattern* PeriodicDisjointDiscovery(PeriodicType periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor);

/*
 * Hellos are piggybacked on regular traffic and are (1-hop) broadcasted if no
 * traffic was sent for some period. Hacks are only periodically (1-hop) broadcasted (with different periodicity than hellos)
 * @Param hack_periodic_type : specifies if the hack timer should be reset on each early hack or not
 * @Param hello_piggyback_filter : hello piggyback type.
 * @Param react_to_new_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_lost_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_update_neighbor : specifies if an hello should be sent when
 * there is any change in some neighbor.
 */
DiscoveryPattern* HybridHelloPeriodicHackDiscovery(PiggybackFilter* hello_piggyback_filter, PeriodicType hack_periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor);

/*
 * Hellos are periodically (1-hop) broadcasted. are piggybacked on regular traffic and are (1-hop) broadcasted if no
 * traffic was sent for some period.
 * @Param hello_periodic_type : specifies if the hello timer should be reset on each early hello or not
 * @Param hack_piggyback_filter : hack piggyback type.
 * @Param react_to_new_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_lost_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_update_neighbor : specifies if an hello should be sent when
 * there is any change in some neighbor.
 */
DiscoveryPattern* PeriodicHelloHybridHackDiscovery(PiggybackFilter* hack_piggyback_filter, PeriodicType hello_periodic_type, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor);

/*
 * Hellos are piggybacked on regular traffic and are (1-hop) broadcasted if no
 * traffic was sent for some period. Hacks are piggybacked on non-piggybacked
 * hellos and are (1-hop) broadcasted if they were not sent for some period.
 * In this later case, an hello is also pigybacked in the message.
 * In case Hacks have a larger period than the Hellos, then there will "long"
 * periodic Hacks (with piggybacked Hellos), when there is traffic, and "short"
 * Hellos (with piggybacked Hacks) when there is not traffic.
 * @Param hello_piggyback_filter : hello piggyback type.
 * @Param hack_piggyback_filter : hack piggyback type.
 * @Param react_to_new_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_lost_neighbor : specifies if an hello should be sent when a
 * new neighbor is found.
 * @Param react_to_update_neighbor : specifies if an hello should be sent when
 * there is any change in some neighbor.
 */
DiscoveryPattern* HybridDisjointDiscovery(PiggybackFilter* hello_piggyback_filter, PiggybackFilter* hack_piggyback_filter, bool react_to_new_neighbor, bool react_to_lost_neighbor, bool react_to_update_neighbor, bool react_to_new_2hop_neighbor, bool react_to_lost_2hop_neighbor, bool react_to_update_2hop_neighbor);

/*
 * Hellos are periodically (1-hop) broadcasted. Hacks are explicitly immediatly
 * replied to the hellos.
 * @Param reply_type : specifies the type of reply to send, defined in  * hacksheduler.h
 * @Param piggyback_hello_on_reply_if_new_neighbor : piggyback an hello on a reply when a
 * new neighbor is found.
 */
DiscoveryPattern* EchoDiscovery(HackReplyType reply_type, bool piggyback_hello_on_reply_if_new_neighbor);

HelloScheduler* DP_getHelloScheduler(DiscoveryPattern* dp);

HackScheduler* DP_getHackScheduler(DiscoveryPattern* dp);

#endif /* _DISCOVERY_PATTERN_H_ */
