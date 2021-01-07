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

#ifndef _ROUTING_UPDATE_H_
#define _ROUTING_UPDATE_H_

#include "routing_table.h"

void RF_updateRoutingTable(RoutingTable* rt, list* to_update, list* to_remove, struct timespec* current_time);

#endif /*_ROUTING_UPDATE_H_*/
