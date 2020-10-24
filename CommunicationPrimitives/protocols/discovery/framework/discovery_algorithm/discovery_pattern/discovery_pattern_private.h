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

#ifndef _DISCOVER_PATTERN_PRIVATE_H_
#define _DISCOVER_PATTERN_PRIVATE_H_

#include "hello_scheduler_private.h"
#include "hack_scheduler_private.h"

#include "discovery_pattern.h"

typedef struct _DiscoveryPattern {
    HelloScheduler* hello_sh;
    HackScheduler* hack_sh;
} DiscoveryPattern;

#endif /* _DISCOVER_PATTERN_PRIVATE_H_ */
