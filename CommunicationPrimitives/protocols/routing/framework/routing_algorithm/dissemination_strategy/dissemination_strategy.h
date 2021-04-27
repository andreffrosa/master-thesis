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

#ifndef _DISSEMINATION_STRATEGY_H_
#define _DISSEMINATION_STRATEGY_H_

#include "../common.h"

typedef struct DisseminationStrategy_ DisseminationStrategy;

void destroyDisseminationStrategy(DisseminationStrategy* ds);

void DS_disseminate(DisseminationStrategy* ds, unsigned char* myID, YggMessage* msg, RoutingEventType event_type, void* info);

///////////////////////////////////////////7

DisseminationStrategy* BroadcastDissemination();

DisseminationStrategy* FisheyeDissemination(unsigned int n_phases, unsigned int phase_radius);

DisseminationStrategy* LocalDissemination();

DisseminationStrategy* ReactiveDissemination(bool hop_delivery);

DisseminationStrategy* ZoneDissemination(unsigned short zone_radius);

#endif /*_DISSEMINATION_STRATEGY_H_*/
