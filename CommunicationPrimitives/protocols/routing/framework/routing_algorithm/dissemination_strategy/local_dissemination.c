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

#include "dissemination_strategy_private.h"

#include "protocols/broadcast/framework/framework.h"

#include <assert.h>

static void disseminate(ModuleState* m_state, YggMessage* msg) {

    unsigned int radius = 1;

    BroadcastMessage(msg->Proto_id, radius, (byte*)msg->data, msg->dataLen);
}

DisseminationStrategy* LocalDissemination() {
    return newDisseminationStrategy(
        NULL,
        NULL,
        &disseminate,
        NULL
    );
}