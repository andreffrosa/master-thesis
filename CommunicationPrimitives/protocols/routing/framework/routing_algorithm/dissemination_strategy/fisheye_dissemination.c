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

typedef struct BroadcastDSArgs_ {
    unsigned int n_phases;
    unsigned int phase_radius;
} BroadcastDSArgs;

static void disseminate(ModuleState* m_state, unsigned char* myID, YggMessage* msg, RoutingEventType event_type, void* info) {

    BroadcastDSArgs* args = (BroadcastDSArgs*)m_state->args;
    unsigned int* current_phase = (unsigned int*)m_state->vars;

    *current_phase = (*current_phase+1) % (args->n_phases + 1);

    unsigned int radius = *current_phase == args->n_phases ? -1 : *current_phase*args->phase_radius;

    BroadcastMessage(msg->Proto_id, radius, 0, (byte*)msg->data, msg->dataLen);
}

DisseminationStrategy* FisheyeDissemination(unsigned int n_phases, unsigned int phase_radius) {

    BroadcastDSArgs* args = malloc(sizeof(BroadcastDSArgs));
    args->n_phases = n_phases;
    args->phase_radius = phase_radius;

    unsigned int* current_phase = malloc(sizeof(unsigned int));
    *current_phase = n_phases;

    return newDisseminationStrategy(
        args,
        current_phase,
        &disseminate,
        NULL
    );
}
