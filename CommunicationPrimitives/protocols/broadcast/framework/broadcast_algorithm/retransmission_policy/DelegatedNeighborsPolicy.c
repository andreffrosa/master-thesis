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

#include <assert.h>

static bool DelegatedNeighborsPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {

    MessageCopy* first_copy = ((MessageCopy*)getCopies(p_msg)->head->data);

    list* delegated_neighbors = NULL;

    list* visited2 = list_init();
    bool found = RC_query(r_context, "delegated_neighbors", &delegated_neighbors, NULL, myID, visited2);
    list_delete(visited2);

    if(!found) {
        list* visited2 = list_init();
        if(!RC_queryHeader(r_context, getContextHeader(first_copy), getBcastHeader(first_copy)->context_length, "delegated_neighbors", &delegated_neighbors, NULL, myID, visited2))
            assert(false);
        list_delete(visited2);
    }

    bool retransmit = list_find_item(delegated_neighbors, &equalID, myID) != NULL;

    // Debug
    printf("Delegated Neighbors:\n");
    for(list_item* it = delegated_neighbors->head; it; it = it->next) {
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(((unsigned char*)it->data), id_str);
        printf("%s\n", id_str);
    }

    list_delete(delegated_neighbors);

	return retransmit;
}

RetransmissionPolicy* DelegatedNeighborsPolicy() {
    return newRetransmissionPolicy(
        NULL,
        NULL,
        &DelegatedNeighborsPolicyEval,
        NULL
    );
}
