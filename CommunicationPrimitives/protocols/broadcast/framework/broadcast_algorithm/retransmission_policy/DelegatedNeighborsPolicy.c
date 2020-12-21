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

static bool DelegatedNeighborsPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {

    MessageCopy* first_copy = ((MessageCopy*)getCopies(p_msg)->head->data);

    bool delegated = false;

    list* visited2 = list_init();
    hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
    char* key = malloc(6*sizeof(char));
    strcpy(key, "p_msg");
    void** value = malloc(sizeof(void*));
    *value = p_msg;
    hash_table_insert(query_args, key, value);
    bool found = RC_queryHeader(r_context, getContextHeader(first_copy), getBcastHeader(first_copy)->context_length, "delegated", &delegated, query_args, myID, visited2);
    list_delete(visited2);

    if(!found) {
        list* visited2 = list_init();
        found = RC_query(r_context, "delegated", &delegated, query_args, myID, visited2);
        list_delete(visited2);
    }
    hash_table_delete(query_args);

    assert(found);

	return delegated;
}

RetransmissionPolicy* DelegatedNeighborsPolicy() {
    return newRetransmissionPolicy(
        NULL,
        NULL,
        &DelegatedNeighborsPolicyEval,
        NULL
    );
}
