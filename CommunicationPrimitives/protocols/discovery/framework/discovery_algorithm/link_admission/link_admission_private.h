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

#ifndef _DISCOVERY_FRAMEWORK_LINK_ADMISSION_PRIVATE_H_
#define _DISCOVERY_FRAMEWORK_LINK_ADMISSION_PRIVATE_H_

#include "link_admission.h"

#include "../common.h"

typedef bool (*la_eval)(ModuleState* state, NeighborEntry* neigh, struct timespec* current_time);

//typedef void* (*lq_create_attrs)(ModuleState* state);

//typedef void (*lq_destroy_attrs)(ModuleState* state, void* lq_attrs);

typedef void (*la_destroy)(ModuleState* state);

typedef struct LinkAdmission_ {
    ModuleState state;

    la_eval eval;

    //la_create_attrs create_attrs;

    //la_destroy_attrs destroy_attrs;

    la_destroy destroy;

} LinkAdmission;

LinkAdmission* newLinkAdmissionPolicy(void* args, void* vars, la_eval eval, /*lq_create_attrs create_attrs, lq_destroy_attrs destroy_attrs,*/ la_destroy destroy);

#endif /* _DISCOVERY_FRAMEWORK_LINK_ADMISSION_PRIVATE_H_ */
