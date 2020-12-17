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

#include "discovery_message_private.h"

#include <assert.h>

DiscoveryMessage* newDiscoveryMessage(void* args, void* vars, d_msg_create_message create_message, d_msg_process_message process_message, d_msg_create_attrs create_attrs, d_msg_destroy_attrs destroy_attrs, d_msg_destroy destroy) {
    assert(create_message && process_message);

    DiscoveryMessage* dm = malloc(sizeof(DiscoveryMessage));

    dm->state.args = args;
    dm->state.vars = vars;
    dm->create_message = create_message;
    dm->process_message = process_message;
    dm->create_attrs = create_attrs;
    dm->destroy_attrs = destroy_attrs;
    dm->destroy = destroy;

    return dm;
}

void destroyDiscoveryMessage(DiscoveryMessage* dm) {
    if(dm) {
        if(dm->destroy) {
            dm->destroy(&dm->state);
        }
        free(dm);
    }
}

void DM_create(DiscoveryMessage* dm, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, DiscoveryInternalEventType event_type, void* event_args, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size) {
    assert(dm);
    dm->create_message(&dm->state, myID, current_time, neighbors, event_type, event_args, hello, hacks, n_hacks, buffer, size);
}

bool DM_process(DiscoveryMessage* dm, void* f_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size, MessageSummary* msg_summary) {
    assert(dm);
    return dm->process_message(&dm->state, f_state, myID, current_time, neighbors, piggybacked, mac_addr, buffer, size, msg_summary);
}

void* DM_createAttrs(DiscoveryMessage* dm) {
    assert(dm);

    if( dm->create_attrs ) {
        return dm->create_attrs(&dm->state);
    } else {
        return NULL;
    }
}

void DM_destroyAttrs(DiscoveryMessage* dm, void* msg_attributes) {
    assert(dm);

    if( msg_attributes ) {
        if( dm->destroy_attrs ) {
            dm->destroy_attrs(&dm->state, msg_attributes);
        } else {
            free(msg_attributes);
        }
    }
}
