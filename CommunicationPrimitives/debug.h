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

#ifndef _DEBUG_UTILITY_H_
#define _DEBUG_UTILITY_H_

#define NO_DEBUG 0
#define SIMPLE_DEBUG 1
#define ADVANCED_DEBUG 2
#define FULL_DEBUG 3

#define DEBUG_INCLUDE_EQ(current_level, statement_level) (current_level == statement_level)
#define DEBUG_INCLUDE_GT(current_level, statement_level) (current_level >= statement_level)

#define DISCOVERY_DEBUG_LEVEL FULL_DEBUG
#define BROADCAST_DEBUG_LEVEL SIMPLE_DEBUG
#define ROUTING_DEBUG_LEVEL SIMPLE_DEBUG

#endif /* _DEBUG_UTILITY_H_ */
