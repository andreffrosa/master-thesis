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

#ifndef _DISCOVERY_MESSAGE_PRIVATE_H_
#define _DISCOVERY_MESSAGE_PRIVATE_H_

#include "discovery_message.h"

typedef bool (*d_msg_create_message)(ModuleState*, unsigned char*, struct timespec*, NeighborsTable*, MessageType, void*, HelloMessage*, HackMessage*, byte, byte*, unsigned short*);

typedef bool (*d_msg_process_message)(ModuleState*, void*, unsigned char*, struct timespec*, NeighborsTable*, bool, WLANAddr*, byte*, unsigned short, MessageSummary*);

typedef void* (*d_msg_create_attrs)(ModuleState* state);

typedef void (*d_msg_destroy_attrs)(ModuleState* state, void* d_msg_attrs);

typedef void (*d_msg_destroy)(ModuleState*);

typedef struct _DiscoveryMessage {
    ModuleState state;

    d_msg_create_message create_message;

    d_msg_process_message process_message;

    d_msg_create_attrs create_attrs;

    d_msg_destroy_attrs destroy_attrs;

    d_msg_destroy destroy;

} DiscoveryMessage;

DiscoveryMessage* newDiscoveryMessage(void* args, void* vars, d_msg_create_message create_message, d_msg_process_message process_message, d_msg_create_attrs create_attrs, d_msg_destroy_attrs destroy_attrs, d_msg_destroy destroy);

#endif /* _DISCOVERY_MESSAGE_PRIVATE_H_ */
