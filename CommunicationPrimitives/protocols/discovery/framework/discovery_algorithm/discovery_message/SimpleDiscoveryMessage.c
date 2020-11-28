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

static bool SDM_createMessage(ModuleState* state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, MessageType msg_type, void* aux_info, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size) {
    byte* ptr = buffer;

    // Serialize Hello
    if( hello ) {
        ptr[0] = 1;
        ptr += 1;
        *size += 1;

        memcpy(ptr, hello, sizeof(HelloMessage));
        ptr += sizeof(HelloMessage);
        *size += sizeof(HelloMessage);
    } else {
        ptr[0] = 0;
        ptr += 1;
        *size += 1;
    }

    // Serialize Hacks
    if( hacks ) {
        ptr[0] = n_hacks;
        ptr += 1;
        *size += 1;

        for(int i = 0; i < n_hacks; i++) {
            memcpy(ptr, &hacks[i], sizeof(HackMessage));
            ptr += sizeof(HackMessage);
            *size += sizeof(HackMessage);
        }
    } else {
        ptr[0] = 0;
        ptr += 1;
        *size += 1;
    }

    return true;
}

static bool SDM_processMessage(ModuleState* state, void* f_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size) {

    byte* ptr = buffer;

    // Deserialize Hello
    byte has_hello = ptr[0];
    ptr += 1;

    if( has_hello ) {
        HelloMessage hello;
        memcpy(&hello, ptr, sizeof(HelloMessage));
        ptr += sizeof(HelloMessage);

        HelloDeliverSummary* summary = deliverHello(f_state, &hello, mac_addr);
        free(summary);
    }

    byte n_hacks = ptr[0];
    ptr += 1;

    // Deserialize Hacks
    if( n_hacks > 0 ) {
        HackMessage hacks[n_hacks];

        for(int i = 0; i < n_hacks; i++) {
            memcpy(&hacks[i], ptr, sizeof(HackMessage));
            ptr += sizeof(HackMessage);

            HackDeliverSummary* summary = deliverHack(f_state, &hacks[i]);
            free(summary);
        }
    }

    return false;
}

/*
static void SDM_destructor(ModuleState* state) {
    // TODO
}
*/

DiscoveryMessage* SimpleDiscoveryMessage() {
    return newDiscoveryMessage(NULL, NULL, &SDM_createMessage, &SDM_processMessage, NULL, NULL, NULL);
}
