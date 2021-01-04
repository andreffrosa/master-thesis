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

#ifndef _DISCOVERY_FRAMEWORK_LINK_ADMISSION_H_
#define _DISCOVERY_FRAMEWORK_LINK_ADMISSION_H_

#include "data_structures/list.h"

#include "../common.h"

typedef struct LinkAdmission_ LinkAdmission;

void destroyLinkAdmissionPolicy(LinkAdmission* lap);

bool LA_eval(LinkAdmission* lap, NeighborEntry* neigh, struct timespec* current_time);

//void* LA_createAttrs(LinkQuality* lqm);

//void LA_destroyAttrs(LinkQuality* lqm, void* lq_attrs);

////////////////////////////////////////////////////////////////////////////

LinkAdmission* NoAdmission();

LinkAdmission* HysteresisAdmission(double hyst_reject, double hyst_accept);

LinkAdmission* AgeAdmission(unsigned int min_hellos);


#endif /* _DISCOVERY_FRAMEWORK_LINK_ADMISSION_H_ */
