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

static bool SBAPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {

    bool all_covered = false;

    list* visited2 = list_init();
    hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
    char* key = malloc(6*sizeof(char));
    strcpy(key, "p_msg");
    void** value = malloc(sizeof(void*));
    *value = p_msg;
    hash_table_insert(query_args, key, value);
    if(!RC_query(r_context, "all_covered", &all_covered, query_args, myID, visited2))
        assert(false);
    hash_table_delete(query_args);
    list_delete(visited2);

    return !all_covered;
}

RetransmissionPolicy* SBAPolicy() {
	return newRetransmissionPolicy(
        NULL,
        NULL,
        &SBAPolicyEval,
        NULL
    );
}
