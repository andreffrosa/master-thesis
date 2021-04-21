/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Author:
 * André Rosa (af.rosa@campus.fct.unl.pt)
 * Under the guidance of:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2020
 *********************************************************/

#include "retransmission_policy_private.h"

#include "utility/my_misc.h"
#include "utility/my_string.h"

#include <assert.h>

static bool BiFloodingPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {

    unsigned int current_phase = getCurrentPhase(p_msg);

    if(current_phase == 1) {
        bool bi = false;

        hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
        char* key = malloc(6*sizeof(char));
        strcpy(key, "p_msg");
        void** value = malloc(sizeof(void*));
        *value = p_msg;
        hash_table_insert(query_args, key, value);

        RetransmissionContext* r_context = hash_table_find_value(contexts, "BiFloodingContext");
        assert(r_context);

        bool found = RC_query(r_context, "bi", &bi, query_args, myID, contexts);
        assert(found);

        hash_table_delete(query_args);

        return bi;
    }

    return false;
}

RetransmissionPolicy* BiFloodingPolicy() {
    return newRetransmissionPolicy(
        NULL,
        NULL,
        &BiFloodingPolicyEval,
        NULL,
        new_list(1, new_str("BiFloodingContext"))
    );

}
