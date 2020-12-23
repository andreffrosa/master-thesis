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

#include "discovery_context_private.h"

#include <assert.h>

DiscoveryContext* newDiscoveryContext(void* args, void* vars, d_ctx_create_message create_message, d_ctx_process_message process_message, d_ctx_update_context update_context, d_ctx_create_attrs create_attrs, d_ctx_destroy_attrs destroy_attrs, d_ctx_destroy destroy) {
    assert(create_message && process_message);

    DiscoveryContext* d_ctx = malloc(sizeof(DiscoveryContext));

    d_ctx->state.args = args;
    d_ctx->state.vars = vars;
    d_ctx->create_message = create_message;
    d_ctx->process_message = process_message;
    d_ctx->update_context = update_context;
    d_ctx->create_attrs = create_attrs;
    d_ctx->destroy_attrs = destroy_attrs;
    d_ctx->destroy = destroy;

    return d_ctx;
}

void destroyDiscoveryContext(DiscoveryContext* d_ctx) {
    if(d_ctx) {
        if(d_ctx->destroy) {
            d_ctx->destroy(&d_ctx->state);
        }
        free(d_ctx);
    }
}

void DC_create(DiscoveryContext* d_ctx, unsigned char* myID, NeighborsTable* neighbors, DiscoveryInternalEventType event_type, void* event_args, struct timespec* current_time, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size) {
    assert(d_ctx);

    d_ctx->create_message(&d_ctx->state, myID, neighbors, event_type, event_args, current_time, hello, hacks, n_hacks, buffer, size);
}

bool DC_process(DiscoveryContext* d_ctx, void* f_state, unsigned char* myID, NeighborsTable* neighbors, struct timespec* current_time, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size, MessageSummary* msg_summary) {
    assert(d_ctx);

    return d_ctx->process_message(&d_ctx->state, f_state, myID, neighbors, current_time, piggybacked, mac_addr, buffer, size, msg_summary);
}

bool DC_update(DiscoveryContext* d_ctx, unsigned char* myID, NeighborEntry* neighbor, NeighborsTable* neighbors, struct timespec* current_time, NeighborTimerSummary* summary) {
    assert(d_ctx);

    if( d_ctx->update_context ) {
        return d_ctx->update_context(&d_ctx->state, myID, neighbor, neighbors, current_time, summary);
    } else {
        return false;
    }
}

void* DC_createAttrs(DiscoveryContext* d_ctx) {
    assert(d_ctx);

    if( d_ctx->create_attrs ) {
        return d_ctx->create_attrs(&d_ctx->state);
    } else {
        return NULL;
    }
}

void DC_destroyAttrs(DiscoveryContext* d_ctx, void* attributes) {
    assert(d_ctx);

    if( attributes ) {
        if( d_ctx->destroy_attrs ) {
            d_ctx->destroy_attrs(&d_ctx->state, attributes);
        } else {
            free(attributes);
        }
    }
}
