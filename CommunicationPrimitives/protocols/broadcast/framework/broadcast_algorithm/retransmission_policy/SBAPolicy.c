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

#include "utility/my_string.h"

#include <assert.h>

static bool SBAPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {
    hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
    char* key = malloc(6*sizeof(char));
    strcpy(key, "p_msg");
    void** value = malloc(sizeof(void*));
    *value = p_msg;
    hash_table_insert(query_args, key, value);

    RetransmissionContext* r_context = hash_table_find_value(contexts, "NeighborsContext");
    assert(r_context);

    bool all_covered = false;
    if(!RC_query(r_context, "all_covered", &all_covered, query_args, myID, contexts))
        assert(false);
    hash_table_delete(query_args);

    return !all_covered;
}

RetransmissionPolicy* SBAPolicy() {
	RetransmissionPolicy* r_policy = newRetransmissionPolicy(
        NULL,
        NULL,
        &SBAPolicyEval,
        NULL,
        new_list(1, new_str("NeighborsContext"))
    );

    return r_policy;
}
