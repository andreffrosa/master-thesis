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

#include "data_structures/hash_table.h"

#include "protocols/discovery/framework/framework.h"

typedef struct LENWBContextArgs_ {
	RetransmissionContext* neighbors_context;
} LENWBContextArgs;

typedef struct LENWBContextState_ {
	hash_table* two_hop_n_neighs;
} LENWBContextState;

static void LENWBContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    LENWBContextArgs* args = (LENWBContextArgs*)context_state->args;

    RC_init(args->neighbors_context, protocol_definition, myID, visited);
}

static void LENWBContextEvent(ModuleState* context_state, queue_t_elem* elem, unsigned char* myID, list* visited) {
    LENWBContextArgs* args = (LENWBContextArgs*)context_state->args;
    LENWBContextState* state = (LENWBContextState*)context_state->vars;

    RC_processEvent(args->neighbors_context, elem, myID, visited);

    if(elem->type == YGG_EVENT) {
        YggEvent* ev = &elem->data.event;
		if(ev->notification_id == GENERIC_DISCOVERY_EVENT) {
            unsigned int str_len = 0;
            void* ptr = NULL;
            ptr = YggEvent_readPayload(ev, ptr, &str_len, sizeof(unsigned int));

            char type[str_len+1];
            ptr = YggEvent_readPayload(ev, ptr, type, str_len*sizeof(char));
            type[str_len] = '\0';

            if( strcmp(type, "LENWB_NEIGHS") == 0 ) {
                hash_table_delete(state->two_hop_n_neighs);

                unsigned int amount = 0;
                ptr = YggEvent_readPayload(ev, ptr, &amount, sizeof(amount));

                state->two_hop_n_neighs = hash_table_init_size(amount, (hashing_function)&uuid_hash, (comparator_function)&equalID);

                for(int i = 0; i < amount; i++) {
                    unsigned char* id = malloc(sizeof(uuid_t));
                    ptr = YggEvent_readPayload(ev, ptr, id, sizeof(uuid_t));

                    byte* n_neighs = malloc(sizeof(byte));
                    ptr = YggEvent_readPayload(ev, ptr, n_neighs, sizeof(byte));

                    hash_table_insert(state->two_hop_n_neighs, id, n_neighs);
                }
            }
        }
    }
}

static bool LENWBContextQuery(ModuleState* context_state, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {
    LENWBContextArgs* args = (LENWBContextArgs*)context_state->args;
    LENWBContextState* state = (LENWBContextState*)context_state->vars;

    if(strcmp(query, "LENWB_NEIGHS") == 0) {
        *((hash_table**)result) = hash_table_clone(state->two_hop_n_neighs, sizeof(uuid_t), sizeof(byte));
		return true;
    } else {
        return RC_query(args->neighbors_context, query, result, query_args, myID, visited);
    }
}

static void LENWBContextDestroy(ModuleState* context_state, list* visited) {
    LENWBContextArgs* args = (LENWBContextArgs*)context_state->args;
    destroyRetransmissionContext(args->neighbors_context, visited);
    free(args);

    LENWBContextState* state = (LENWBContextState*)context_state->vars;
    hash_table_delete(state->two_hop_n_neighs);
    free(state);
}

RetransmissionContext* LENWBContext(RetransmissionContext* neighbors_context) {

    LENWBContextArgs* args = malloc(sizeof(LENWBContextArgs));
    args->neighbors_context = neighbors_context;

    LENWBContextState* state = malloc(sizeof(LENWBContextState));
    state->two_hop_n_neighs = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    return newRetransmissionContext(
        args,
        state,
        &LENWBContextInit,
        &LENWBContextEvent,
        NULL,
        &LENWBContextQuery,
        NULL,
        NULL,
        &LENWBContextDestroy
    );
}
