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

#ifndef _DISSEMINATION_STRATEGY_PRIVATE_H_
#define _DISSEMINATION_STRATEGY_PRIVATE_H_

#include "dissemination_strategy.h"

typedef void (*ds_disseminate)(ModuleState*, YggMessage*, RoutingEventType);

typedef void (*ds_destroy)(ModuleState*);

typedef struct DisseminationStrategy_ {
    ModuleState state;

    ds_disseminate disseminate;

    ds_destroy destroy;
} DisseminationStrategy;

DisseminationStrategy* newDisseminationStrategy(void* args, void* vars, ds_disseminate disseminate, ds_destroy destroy);

#endif /*_DISSEMINATION_STRATEGY_PRIVATE_H_*/
