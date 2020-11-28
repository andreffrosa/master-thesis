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

static bool _DelegatedNeighborsPolicy(ModuleState* policy_state, PendingMessage* p_msg, RetransmissionContext* r_context, unsigned char* myID) {

    message_copy* first_copy = ((message_copy*)getCopies(p_msg)->head->data);

    list* delegated_neighbors = NULL;
    if(!query_context_header(r_context, getContextHeader(first_copy), getBcastHeader(first_copy)->context_length, "delegated_neighbors", &delegated_neighbors, myID, 0))
        assert(false);

    bool retransmit = false;

    retransmit = list_find_item(delegated_neighbors, &equalID, myID) != NULL;

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
	RetransmissionPolicy* r_policy = malloc(sizeof(RetransmissionPolicy));

	r_policy->policy_state.args = NULL;
	r_policy->policy_state.vars = NULL;

	r_policy->r_policy = &_DelegatedNeighborsPolicy;
    r_policy->destroy = NULL;

	return r_policy;
}
