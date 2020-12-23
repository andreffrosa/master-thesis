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

static bool DelegatedNeighborsPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {

    MessageCopy* first_copy = ((MessageCopy*)getCopies(p_msg)->head->data);
    hash_table* headers = getHeaders(first_copy);

    bool delegated = false;

    list* delegated_neighbors = (list*)hash_table_find_value(headers, "delegated_neighbors");
    if(delegated_neighbors) {
        delegated = list_find_item(delegated_neighbors, &equalID, myID) != NULL;
    } else {
        hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
        char* key = malloc(6*sizeof(char));
        strcpy(key, "p_msg");
        void** value = malloc(sizeof(void*));
        *value = p_msg;
        hash_table_insert(query_args, key, value);

        // TODO: Search in all contexts
        RetransmissionContext* r_context = hash_table_find_value(contexts, "MultiPointRelayContext");
        assert(r_context);

        bool found = RC_query(r_context, "delegated", &delegated, query_args, myID, contexts);
        assert(found);

        hash_table_delete(query_args);
    }

	return delegated;
}

RetransmissionPolicy* DelegatedNeighborsPolicy() {
    return newRetransmissionPolicy(
        NULL,
        NULL,
        &DelegatedNeighborsPolicyEval,
        NULL,
        NULL
    );

    // TODO: how to specify the dependencies of this?
}
