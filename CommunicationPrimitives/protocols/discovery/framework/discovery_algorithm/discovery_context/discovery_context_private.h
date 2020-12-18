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

#ifndef _DISCOVERY_CONTEXT_PRIVATE_H_
#define _DISCOVERY_CONTEXT_PRIVATE_H_

#include "discovery_context.h"

typedef void (*d_ctx_create_message)(ModuleState*, unsigned char*, NeighborsTable*, DiscoveryInternalEventType, void*, struct timespec*, HelloMessage*, HackMessage*, byte, byte*, unsigned short*);

typedef bool (*d_ctx_process_message)(ModuleState*, void*, unsigned char*, NeighborsTable*, struct timespec*, bool, WLANAddr*, byte*, unsigned short, MessageSummary*);

typedef bool (*d_ctx_update_context)(ModuleState*, unsigned char*, NeighborEntry*, NeighborsTable*, struct timespec*, NeighborTimerSummary*);

typedef void* (*d_ctx_create_attrs)(ModuleState* );

typedef void (*d_ctx_destroy_attrs)(ModuleState*, void*);

typedef void (*d_ctx_destroy)(ModuleState*);

typedef struct DiscoveryContext_ {
    ModuleState state;

    d_ctx_create_message create_message;

    d_ctx_process_message process_message;

    d_ctx_update_context update_context;

    d_ctx_create_attrs create_attrs;

    d_ctx_destroy_attrs destroy_attrs;

    d_ctx_destroy destroy;
} DiscoveryContext;

DiscoveryContext* newDiscoveryContext(void* args, void* vars, d_ctx_create_message create_message, d_ctx_process_message process_message, d_ctx_update_context update_context, d_ctx_create_attrs create_attrs, d_ctx_destroy_attrs destroy_attrs, d_ctx_destroy destroy);


#endif /*_DISCOVERY_CONTEXT_PRIVATE_H_*/
