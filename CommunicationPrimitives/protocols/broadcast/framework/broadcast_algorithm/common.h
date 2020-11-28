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

#ifndef _BROADCAST_ALGORITHM_COMMON_H_
#define _BROADCAST_ALGORITHM_COMMON_H_

#include "Yggdrasil.h"
#include "../pending_messages/pending_message.h"

#include "../bcast_header.h"

typedef struct ModuleState_ {
    void* args;
    void* vars;
} ModuleState;

bool equalAddr(void* a, void* b);

#endif /* _BROADCAST_ALGORITHM_COMMON_H_ */
