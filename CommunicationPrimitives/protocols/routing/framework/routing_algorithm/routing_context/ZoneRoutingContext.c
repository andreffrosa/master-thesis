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

typedef struct ZoneRoutingArgs_ {
    RoutingContext* proactive_ctx;
    RoutingContext* reactive_ctx;
} ZoneRoutingArgs;

/*static void ZoneRoutingContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time) {

}*/


static RoutingContextSendType ZoneRoutingContextTriggerEvent(ModuleState* m_state, const char* proto, RoutingEventType event_type, void* info, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {
    ZoneRoutingArgs* args = (ZoneRoutingArgs*)m_state->args;

    RoutingContextSendType send_type = NO_SEND;

    if(event_type == RTE_NEIGHBORS_CHANGE || event_type == RTE_ANNOUNCE_TIMER || event_type == RTE_SOURCE_EXPIRE) {
        send_type = RCtx_triggerEvent(args->proactive_ctx, event_type, info, routing_table, neighbors, source_table, myID, current_time);
    } else {
        send_type = RCtx_triggerEvent(args->reactive_ctx, event_type, info, routing_table, neighbors, source_table, myID, current_time);
    }

    return send_type;
}

static RoutingContextSendType ZoneRoutingContextCreateMsg(ModuleState* m_state, const char* proto, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info) {

    ZoneRoutingArgs* args = (ZoneRoutingArgs*)m_state->args;

    byte type = 0;

    if(event_type == RTE_NEIGHBORS_CHANGE || event_type == RTE_ANNOUNCE_TIMER || event_type == RTE_SOURCE_EXPIRE) {
        type = 1;
        YggMessage_addPayload(msg, (char*)&type, sizeof(byte));
        return RCtx_createMsg(args->proactive_ctx, header, routing_table, neighbors, source_table, myID, current_time, msg, event_type, info);
    } else {
        if(event_type == RTE_CONTROL_MESSAGE) {
            assert(info);
            //SourceEntry* entry = ((void**)info)[0];
            byte* payload = ((void**)info)[1];
            unsigned short length = *((unsigned short*)((void**)info)[2]);

            unsigned short new_len = length - sizeof(byte);
            byte aux[new_len];
            memcpy(aux, payload+sizeof(byte), new_len);

            ((void**)info)[1] = aux;
            ((void**)info)[2] = &new_len;

            type = 2;
            YggMessage_addPayload(msg, (char*)&type, sizeof(byte));
            return RCtx_createMsg(args->reactive_ctx, header, routing_table, neighbors, source_table, myID, current_time, msg, event_type, info);
        } else {
            type = 2;
            YggMessage_addPayload(msg, (char*)&type, sizeof(byte));
            return RCtx_createMsg(args->reactive_ctx, header, routing_table, neighbors, source_table, myID, current_time, msg, event_type, info);
        }

    }

    return NO_SEND;

    // TODO: assim?
}

static RoutingContextSendType ZoneRoutingContextProcessMsg(ModuleState* m_state, const char* proto, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, unsigned short src_proto, byte* meta_data, unsigned int meta_length, bool new_seq, bool new_source, unsigned short my_seq, void* f_state) {

    if(!new_seq) {
        return NO_SEND;
    }

    ZoneRoutingArgs* args = (ZoneRoutingArgs*)m_state->args;

    byte type = 0;
    memcpy(&type, payload, sizeof(byte));
    // assert(type > 0);

    unsigned short new_len = length - sizeof(byte);
    byte aux[new_len];
    memcpy(aux, payload+sizeof(byte), new_len);

    //printf("ZONE TYPE: %d    %d bytes\n", type, new_len);

    if(type == 1) {
        return RCtx_processMsg(args->proactive_ctx, routing_table, neighbors, source_table, source_entry, myID, current_time, header, aux, new_len, src_proto, meta_data, meta_length, new_seq, new_source, my_seq, f_state);
    } else if(type == 2) {
        return RCtx_processMsg(args->reactive_ctx, routing_table, neighbors, source_table, source_entry, myID, current_time, header, aux, new_len, src_proto, meta_data, meta_length, new_seq, new_source, my_seq, f_state);
    } else {
        assert(false);
    }

    return NO_SEND;
}


RoutingContext* ZoneRoutingContext(RoutingContext* proactive_ctx, RoutingContext* reactive_ctx) {

    ZoneRoutingArgs* args = malloc(sizeof(ZoneRoutingArgs));
    args->proactive_ctx = proactive_ctx;
    args->reactive_ctx = reactive_ctx;

    return newRoutingContext(
        "ZONE",
        args,
        NULL,
        NULL, //&StaticRoutingContextInit,
        &ZoneRoutingContextTriggerEvent,
        &ZoneRoutingContextCreateMsg,
        &ZoneRoutingContextProcessMsg,
        NULL
    );
}
