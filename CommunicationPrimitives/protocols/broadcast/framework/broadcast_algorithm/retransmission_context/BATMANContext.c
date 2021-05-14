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
 * (C) 2019
 *********************************************************/

#include "retransmission_context_private.h"

#include "utility/my_math.h"
#include "utility/my_string.h"

//#include "protocols/routing/framework/framework.h"
#define ROUTING_FRAMEWORK_PROTO_ID 161
#define EV_ROUTING_TABLE 1

#include <assert.h>

typedef struct RoutingDest_ {
    uuid_t dest_id;
    uuid_t next_hop_id;
    //unsigned short hops;
} RoutingDest;

typedef struct BATMANState_ {
    hash_table* destinations;
} BATMANState;

static void BATMANContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID) {
    proto_def_add_consumed_event(protocol_definition, ROUTING_FRAMEWORK_PROTO_ID, EV_ROUTING_TABLE);
}

static void BATMANContextEvent(ModuleState* context_state, queue_t_elem* elem, unsigned char* myID, hash_table* contexts) {
    BATMANState* state = (BATMANState*) (context_state->vars);

    if(elem->type == YGG_EVENT) {
        YggEvent* ev = &elem->data.event;

        if( ev->proto_origin == ROUTING_FRAMEWORK_PROTO_ID ) {
            unsigned short ev_id = ev->notification_id;

            bool process = ev_id == EV_ROUTING_TABLE;
            if(process) {

                hash_table_delete(state->destinations);
                state->destinations = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

                unsigned short read = 0;
                unsigned char* ptr = ev->payload;

                byte amount = 0;
                memcpy(&amount, ptr, sizeof(byte));
            	ptr += sizeof(byte);
                read += sizeof(byte);

                for(int i = 0; i < amount; i++) {
                    uuid_t dest_id;
                    memcpy(dest_id, ptr, sizeof(uuid_t));
                    ptr += sizeof(uuid_t);
                    read += sizeof(uuid_t);

                    uuid_t next_hop_id;
                    memcpy(next_hop_id, ptr, sizeof(uuid_t));
                    ptr += sizeof(uuid_t);
                    read += sizeof(uuid_t);

                    ptr += WLAN_ADDR_LEN;
                    read += WLAN_ADDR_LEN;

                    ptr += sizeof(double);
                    read += sizeof(double);

                    //byte hops = 0;
                    //memcpy(&hops, ptr, sizeof(byte));
                    ptr += sizeof(byte);
                    read += sizeof(byte);

                    RoutingDest* d = malloc(sizeof(RoutingDest));
                    uuid_copy(d->dest_id, dest_id);
                    uuid_copy(d->next_hop_id, next_hop_id);
                    //d->hops = hops;
                    hash_table_insert(state->destinations, new_id(dest_id), d);
                }
            }
        }
    }
}

static bool BATMANContextQuery(ModuleState* context_state, const char* query, void* result, hash_table* query_args, unsigned char* myID, hash_table* contexts) {
	BATMANState* state = (BATMANState*) (context_state->vars);

	if(strcmp(query, "best_neigh") == 0) {
        assert(query_args);
        void** aux1 = hash_table_find_value(query_args, "p_msg");
        assert(aux1);
		PendingMessage* p_msg = *aux1;

        unsigned char* dest_id = getBcastHeader(getCopies(p_msg)->head->data)->source_id;

        RoutingDest* rd = hash_table_find_value(state->destinations, dest_id);
        if(rd) {
            uuid_copy((unsigned char*)result, rd->next_hop_id);
            return true;
        } else {
            return false;
        }
    } else {
		return false;
	}
}

static void BATMANContextDestroy(ModuleState* context_state) {

    BATMANState* state = (BATMANState*) (context_state->vars);
    hash_table_delete(state->destinations);
    free(state);
}

RetransmissionContext* BATMANContext() {

    BATMANState* state = malloc(sizeof(BATMANState));

    state->destinations = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    return newRetransmissionContext(
        "BATMANContext",
        NULL,
        state,
        &BATMANContextInit,
        &BATMANContextEvent,
        NULL,
        NULL,
        &BATMANContextQuery,
        NULL,
        &BATMANContextDestroy,
        NULL
    );
}
