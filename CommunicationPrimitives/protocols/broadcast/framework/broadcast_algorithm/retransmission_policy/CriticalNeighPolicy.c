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
 * (C) 2020
 *********************************************************/

#include "retransmission_policy_private.h"

#include <assert.h>

#include "utility/my_string.h"

static bool CriticalNeighPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
	double min_critical_coverage = *((double*)policy_state->args);
	unsigned int current_phase = getCurrentPhase(p_msg);

    // Check if the current node is critical to all the parents. In case it is not cirtical to some parent then all the neighbors already were convered.

    bool isCritical = true;

    double_list* copies = getCopies(p_msg);
    for(double_list_item* it = copies->head; it; it = it->next) {
        unsigned char* parent_id = getBcastHeader((MessageCopy*)it->data)->sender_id;

        list* visited2 = list_init();
        NeighCoverageLabel in_label;
        hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
        char* key = malloc(3*sizeof(char));
        strcpy(key, "id");
        unsigned char* value = malloc(sizeof(uuid_t));
        uuid_copy(value, parent_id);
        hash_table_insert(query_args, key, value);
        if(!RC_query(r_context, "node_in_label", &in_label, query_args, myID, visited2))
            assert(false);
        hash_table_delete(query_args);
        list_delete(visited2);

        isCritical &= (in_label == CRITICAL_NODE || in_label == UNKNOWN_NODE);
    }

	if ( isCritical ) { // Only the critical  processes retransmit
		if(current_phase == 1) {
			return true;
		} else {
            // May lead to incorrect assumptions on the necessity of other nodes retransmiting, because some other node may turn them covered or redundant, and thus it increases the cost of the second phase
            double missed_critical_neighs = 0.0;

            list* visited2 = list_init();
            hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
            char* key = malloc(6*sizeof(char));
            strcpy(key, "p_msg");
            void** value = malloc(sizeof(void*));
            *value = p_msg;
            hash_table_insert(query_args, key, value);
            if(!RC_query(r_context, "missed_critical_neighs", &missed_critical_neighs, query_args, myID, visited2))
                assert(false);
            hash_table_delete(query_args);
            list_delete(visited2);

			if(missed_critical_neighs > min_critical_coverage)
				return true;
		}
	}
	return false;
}

static void CriticalNeighPolicyDestroy(ModuleState* policy_state, list* visited) {
    free(policy_state->args);
}

RetransmissionPolicy* CriticalNeighPolicy(double min_critical_coverage) {
	assert( 0.0 <= min_critical_coverage && min_critical_coverage  <= 1.0 );

    double* arg = malloc(sizeof(double));
    *arg = min_critical_coverage;

    return newRetransmissionPolicy(
        arg,
        NULL,
        &CriticalNeighPolicyEval,
        &CriticalNeighPolicyDestroy
    );
}
