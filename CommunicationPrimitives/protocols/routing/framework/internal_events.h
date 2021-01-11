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

#ifndef _ROUTING_INTERNAL_EVENTS_H_
#define _ROUTING_INTERNAL_EVENTS_H_

typedef enum {
    RTE_ANNOUNCE_TIMER,
    RTE_NEIGHBORS_CHANGE,
    RTE_CONTROL_MESSAGE,
    RTE_SOURCE_EXPIRE
} RoutingEventType;


#endif /*_ROUTING_INTERNAL_EVENTS_H_*/
