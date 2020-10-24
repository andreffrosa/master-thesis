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

typedef enum {
    NULL_MPR,
    FLOODING_MPR,
    ROUTING_MPR,
    FLOOD_ROUTE_MPR
} NeighMPRType;

typedef struct _OLSRAttrs {
    bool flooding_mpr;
    bool flooding_mpr_selector; // L_mpr_selector
    bool routing_mpr;
    bool routing_mpr_selector; // N_mpr_selector
    // bool advertised;
} OLSRAttrs;

static void* OLSR_createAttrs(ModuleState* state) {
    OLSRAttrs* attrs = malloc(sizeof(OLSRAttrs));

    attrs->flooding_mpr = false;
    attrs->flooding_mpr_selector = false;
    attrs->routing_mpr = false;
    attrs->routing_mpr_selector = false;
    // attrs->advertised = false;

    return attrs;
}

/*
static void OLSR_destroyAttrs(ModuleState* state, void* d_msg_attrs) {

}
*/

static void OLSR_createMessage(ModuleState* state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size) {
    assert(hello);

    byte* ptr = buffer;

    // Serialize Hello
    memcpy(ptr, hello, sizeof(HelloMessage));
    ptr += sizeof(HelloMessage);
    *size += sizeof(HelloMessage);

    // Serialize Hacks
    ptr[0] = n_hacks;
    ptr += 1;
    *size += 1;

    for(int i = 0; i < n_hacks; i++) {
        memcpy(ptr, &hacks[i], sizeof(HackMessage));
        ptr += sizeof(HackMessage);
        *size += sizeof(HackMessage);

        NeighborEntry* neigh = NT_getNeighbor(neighbors, hacks[i].src_process_id);
        OLSRAttrs* neigh_attrs = NE_getMessageAttributes(neigh);

        if( hacks[i].neigh_type != BI_NEIGH ) {
            neigh_attrs->flooding_mpr = false;
            neigh_attrs->flooding_mpr_selector = false;
            neigh_attrs->routing_mpr = false;
            neigh_attrs->routing_mpr_selector = false;
        }

        NeighMPRType mpr_type;
        if( neigh_attrs->flooding_mpr && !neigh_attrs->routing_mpr ) {
            mpr_type = FLOODING_MPR;
        } else if( !neigh_attrs->flooding_mpr && neigh_attrs->routing_mpr ) {
            mpr_type = ROUTING_MPR;
        } else if( neigh_attrs->flooding_mpr && neigh_attrs->routing_mpr ) {
            mpr_type = FLOOD_ROUTE_MPR;
        } else {
            mpr_type = NULL_MPR;
        }

        byte aux = mpr_type;
        memcpy(ptr, &aux, sizeof(aux));
        ptr += sizeof(aux);
        *size += sizeof(aux);
    }

}

static bool OLSR_processMessage(ModuleState* state, void* f_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size) {
    byte* ptr = buffer;

    // Deserialize Hello
    HelloMessage hello;
    memcpy(&hello, ptr, sizeof(HelloMessage));
    ptr += sizeof(HelloMessage);

    deliverHello(f_state, &hello, mac_addr);

    // Deserialize Hacks
    byte n_hacks = ptr[0];
    ptr += 1;

    if( n_hacks > 0 ) {
        HackMessage hacks[n_hacks];

        for(int i = 0; i < n_hacks; i++) {
            memcpy(&hacks[i], ptr, sizeof(HackMessage));
            ptr += sizeof(HackMessage);

            deliverHack(f_state, &hacks[i]);

            byte aux;
            memcpy(&aux, ptr, sizeof(aux));
            ptr += sizeof(aux);
            NeighMPRType mpr_type = aux;

            if( uuid_compare(hacks[i].dest_process_id, myID) == 0 ) {
                NeighborEntry* neigh = NT_getNeighbor(neighbors, hacks[i].src_process_id);
                OLSRAttrs* neigh_attrs = NE_getMessageAttributes(neigh);

                if( mpr_type == FLOODING_MPR || mpr_type == FLOOD_ROUTE_MPR ) {
                    neigh_attrs->flooding_mpr_selector = true;
                } else {
                    neigh_attrs->flooding_mpr_selector = false;
                }

                if( mpr_type == ROUTING_MPR || mpr_type == FLOOD_ROUTE_MPR ) {
                    neigh_attrs->routing_mpr_selector = true;
                } else {
                    neigh_attrs->routing_mpr_selector = false;
                }
            }
        }
    }

    return false;
}

/*
static void OLSR_destructor(ModuleState* state) {
    // TODO
}
*/

DiscoveryMessage* OLSRDiscoveryMessage() {
    return newDiscoveryMessage(NULL, NULL, &OLSR_createMessage, &OLSR_processMessage, &OLSR_createAttrs, NULL, NULL);
}
