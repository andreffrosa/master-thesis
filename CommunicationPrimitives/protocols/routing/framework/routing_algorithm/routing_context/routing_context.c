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

#include "routing_context_private.h"

#include <assert.h>

RoutingContext* newRoutingContext(void* args, void* vars, rc_init init, rc_triggerEvent trigger_event, rc_createMsg create_msg, rc_processMsg process_msg, rc_destroy destroy) {

    RoutingContext* rc = malloc(sizeof(RoutingContext));

    rc->state.args = args;
    rc->state.vars = vars;
    rc->init = init;
    rc->trigger_event = trigger_event;
    rc->create_msg = create_msg;
    rc->process_msg = process_msg;
    rc->destroy = destroy;

    return rc;
}

void destroyRoutingContext(RoutingContext* r_context) {

    if(r_context !=NULL) {
        if(r_context->destroy != NULL) {
            r_context->destroy(&r_context->state);
        }
        free(r_context);
    }
}

void RCtx_init(RoutingContext* context, proto_def* protocol_definition, unsigned char* myID, RoutingTable* r_table, struct timespec* current_time) {
    assert(context);

    if(context->init)
        context->init(&context->state, protocol_definition, myID, r_table, current_time);
}

RoutingContextSendType RCtx_triggerEvent(RoutingContext* context, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {
    assert(context);

    if(context->trigger_event) {
        return context->trigger_event(&context->state, event_type, args, routing_table, neighbors, source_table, myID, current_time);
    } else {
        return false;
    }
}

void RCtx_createMsg(RoutingContext* context, unsigned short seq, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg) {
    assert(context);

    if(context->create_msg) {
        context->create_msg(&context->state, seq, routing_table, neighbors, source_table, myID, current_time, msg);
    }
}

void RCtx_processMsg(RoutingContext* context, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg) {
    assert(context);

    if(context->process_msg) {
        context->process_msg(&context->state, routing_table, neighbors, source_table, myID, current_time, msg);
    }
}
