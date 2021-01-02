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

#include "protocols/discovery/framework/framework.h"

#include <assert.h>

typedef struct OLSRState_ {
    list* mprs;
    list* mpr_selectors;
} OLSRState;

static void OLSRRoutingContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, RoutingTable* routing_table, struct timespec* current_time) {

}

static bool OLSRRoutingContextTriggerEvent(ModuleState* m_state, unsigned short seq, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, unsigned char* myID, YggMessage* msg) {

    OLSRState* state = (OLSRState*)m_state->vars;

    if(event_type == RTE_NEIGHBORS_CHANGE) {
        YggEvent* ev = args;
        assert(ev);

        if(ev->notification_id == GENERIC_DISCOVERY_EVENT) {

            unsigned int str_len = 0;
            void* ptr = NULL;
            ptr = YggEvent_readPayload(ev, ptr, &str_len, sizeof(unsigned int));

            char type[str_len+1];
            ptr = YggEvent_readPayload(ev, ptr, type, str_len*sizeof(char));
            type[str_len] = '\0';

            if( strcmp(type, "MPRS") == 0 || strcmp(type, "MPR SELECTORS") == 0 ) {
                unsigned int amount = 0;
                ptr = YggEvent_readPayload(ev, ptr, &amount, sizeof(unsigned int));

                // Skip Flooding
                ptr += amount*sizeof(uuid_t);

                amount = 0;
                ptr = YggEvent_readPayload(ev, ptr, &amount, sizeof(unsigned int));

                list* l = list_init();

                for(int i = 0; i < amount; i++) {
                    //uuid_t id;
                    unsigned char* id = malloc(sizeof(uuid_t));
                    ptr = YggEvent_readPayload(ev, ptr, id, sizeof(uuid_t));

                    list_add_item_to_tail(l, id);

                    /*
                    char id_str[UUID_STR_LEN+1];
                    id_str[UUID_STR_LEN] = '\0';
                    uuid_unparse(id, id_str);
                    printf("%s\n", id_str);
                    */
                }

                if( strcmp(type, "MPRS") == 0  ) {
                    list_delete(state->mprs);
                    state->mprs = l;
                } else if( strcmp(type, "MPR SELECTORS") == 0 ) {
                    list_delete(state->mpr_selectors);
                    state->mpr_selectors = l;
                }
            }
    	}

        return false;
    }

    else if(event_type == RTE_ANNOUNCE_TIMER) {

        YggMessage_addPayload(msg, (char*)myID, sizeof(uuid_t));

        YggMessage_addPayload(msg, (char*)&seq, sizeof(unsigned short));

        byte period = 5; // TODO: como obter o period?
        YggMessage_addPayload(msg, (char*)&period, sizeof(byte));

        byte amount = state->mpr_selectors->size;
        YggMessage_addPayload(msg, (char*)&amount, sizeof(byte));

        for(list_item* it = state->mpr_selectors->head; it; it = it->next) {
            unsigned char* id = (unsigned char*)it->data;

            RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, id);
            assert(neigh);
            double tx_cost = RNE_getCost(neigh);

            YggMessage_addPayload(msg, (char*)id, sizeof(uuid_t));
            YggMessage_addPayload(msg, (char*)&tx_cost, sizeof(double));
        }

        return true;
    }

    return false;
}

static void OLSRRoutingContextRcvMsg(ModuleState* m_state, RoutingTable* routing_table, RoutingNeighbors* neighbors, YggMessage* msg) {
    void* ptr = NULL;

    unsigned short payload_size = 0;
    ptr = YggMessage_readPayload(msg, ptr, &payload_size, sizeof(payload_size)); // Bcast framework

    uuid_t src;
    ptr = YggMessage_readPayload(msg, ptr, src, sizeof(uuid_t));

    unsigned short seq = 0;
    ptr = YggMessage_readPayload(msg, ptr, &seq, sizeof(unsigned short));

    char id_str[UUID_STR_LEN+1];
    id_str[UUID_STR_LEN] = '\0';
    uuid_unparse(src, id_str);
    printf("Received TC message from %s with seq %hu\n", id_str, seq);
    fflush(stdout);

    // TODO
}

//typedef void (*rc_destroy)(ModuleState* m_state);


RoutingContext* OLSRRoutingContext() {

    OLSRState* state = malloc(sizeof(OLSRState));
    state->mprs = list_init();
    state->mpr_selectors = list_init();

    return newRoutingContext(
        NULL,
        state,
        &OLSRRoutingContextInit,
        &OLSRRoutingContextTriggerEvent,
        &OLSRRoutingContextRcvMsg,
        NULL
    );
}
