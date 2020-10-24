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

#ifndef _DISCOVERY_FRAMEWORK_LINK_QUALITY_PRIVATE_H_
#define _DISCOVERY_FRAMEWORK_LINK_QUALITY_PRIVATE_H_

#include "link_quality.h"

#include "../common.h"

typedef double (*lq_compute)(ModuleState* state, void* lq_attrs, double previous_link_quality, unsigned int received, unsigned int lost, bool init, struct timespec* current_time);

typedef void* (*lq_create_attrs)(ModuleState* state);

typedef void (*lq_destroy_attrs)(ModuleState* state, void* lq_attrs);

typedef void (*lq_destroy)(ModuleState* state/*, list* visited*/);

typedef struct _LinkQuality {
    ModuleState state;

    lq_compute compute;

    lq_create_attrs create_attrs;

    lq_destroy_attrs destroy_attrs;

    lq_destroy destroy;

} LinkQuality;

LinkQuality* newLinkQualityMetric(void* args, void* vars, lq_compute compute, lq_create_attrs create_attrs, lq_destroy_attrs destroy_attrs, lq_destroy destroy);

#endif /* _DISCOVERY_FRAMEWORK_LINK_QUALITY_PRIVATE_H_ */
