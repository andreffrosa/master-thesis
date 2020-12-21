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

#include "retransmission_policy_private.h"

#include <assert.h>

typedef struct CountParentsArgs_ {
    unsigned int c;
    bool count_same_parents;
} CountParentsArgs;

static bool CountParentsPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
    CountParentsArgs* args = (CountParentsArgs*)policy_state->args;

    unsigned int counter = 0;

    uuid_t first_parent;

    for(double_list_item* dit = getCopies(p_msg)->head; dit; dit = dit->next) {
        MessageCopy* copy = (MessageCopy*)dit->data;

        list* parents = NULL;

        list* visited2 = list_init();
        if(!RC_queryHeader(r_context, getContextHeader(copy), getBcastHeader(copy)->context_length, "parents", &parents, NULL, myID, visited2)) {
            assert(false);
        }
        list_delete(visited2);

        if(parents->head) {
            unsigned char* parent = (unsigned char*)parents->head->data;

            if( dit == getCopies(p_msg)->head ) {
                uuid_copy(first_parent, parent);
                counter++;
            } else {
                int diff = uuid_compare(parent, first_parent);
                if( (args->count_same_parents ? diff == 0 : diff != 0 ) ) {
                    counter++;
                }
            }

        }

        list_delete(parents);
    }


	return counter < args->c;
}

static void CountParentsPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* CountParentsPolicy(unsigned int c, bool count_same_parents) {

    CountParentsArgs* args = malloc(sizeof(CountParentsArgs));
    args->c = c;
    args->count_same_parents = count_same_parents;

    return newRetransmissionPolicy(
        args,
        NULL,
        &CountParentsPolicyEval,
        &CountParentsPolicyDestroy
    );
}
