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

typedef struct CriticalNeighPolicyArgs_ {
    bool all_copies;
    bool mobility_extension;
    double min_critical_coverage;
} CriticalNeighPolicyArgs;

static NeighCoverageLabel getInLabel(unsigned char* parent_id, unsigned char* myID, hash_table* contexts) {

    hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
    char* key = malloc(3*sizeof(char));
    strcpy(key, "id");
    unsigned char* value = malloc(sizeof(uuid_t));
    uuid_copy(value, parent_id);
    hash_table_insert(query_args, key, value);

    RetransmissionContext* r_context = hash_table_find_value(contexts, "LabelNeighsContext");
    assert(r_context);

    NeighCoverageLabel in_label;
    if(!RC_query(r_context, "node_in_label", &in_label, query_args, myID, contexts))
    assert(false);

    hash_table_delete(query_args);

    return in_label;
}

static double getMissedCriticalNeighs(PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {

    hash_table* query_args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);
    char* key = malloc(6*sizeof(char));
    strcpy(key, "p_msg");
    void** value = malloc(sizeof(void*));
    *value = p_msg;
    hash_table_insert(query_args, key, value);

    RetransmissionContext* r_context = hash_table_find_value(contexts, "LabelNeighsContext");
    assert(r_context);

    double missed_critical_neighs = 0.0;
    if(!RC_query(r_context, "missed_critical_neighs", &missed_critical_neighs, query_args, myID, contexts))
    assert(false);

    hash_table_delete(query_args);

    return missed_critical_neighs;
}

static bool CriticalNeighPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {

    CriticalNeighPolicyArgs* args = (CriticalNeighPolicyArgs*)policy_state->args;

    unsigned int current_phase = getCurrentPhase(p_msg);

    // Check if current node is not critical to at least one of its parents. In case this is true, then all the neighbors were already covered.

    bool all_neighs_covered = false;
    bool found_unknown_node = false;

    double_list* copies = getCopies(p_msg);

    if( args->all_copies ) {
        for(double_list_item* it = copies->head; it; it = it->next) {
            MessageCopy* copy = (MessageCopy*)it->data;
            unsigned char* parent_id = getBcastHeader(copy)->sender_id;
            unsigned int phase = getCopyPhase(copy);

            if(phase == 1) {
                NeighCoverageLabel in_label = getInLabel(parent_id, myID, contexts);

                all_neighs_covered |= in_label != CRITICAL_NODE;

                found_unknown_node |= in_label == UNKNOWN_NODE;
            }

        }
    } else {
        MessageCopy* first_copy = (MessageCopy*)copies->head->data;
        unsigned char* parent_id = getBcastHeader(first_copy)->sender_id;

        NeighCoverageLabel in_label = getInLabel(parent_id, myID, contexts);

        all_neighs_covered |= in_label != CRITICAL_NODE;

        found_unknown_node |= in_label == UNKNOWN_NODE;

        //printf("is_critical: %s unknwon %s\n", (!all_neighs_covered?"true":"false"), (found_unknown_node?"true":"false"));
    }

    if(current_phase == 1) {
        return !all_neighs_covered || (found_unknown_node && args->mobility_extension);
    } else {
        // Nodes that retransmited on the first phase, verify if recevived copies from its neighbors whom consider themselves to be critical to the first. In case the fraction of missed copies is above a certain threshold, then node retransmit again. However, nodes that have neighbors whom consider to be critical to them may be covered or redundant by some other node and thus not retransmitting. Therefore, in the subsequent phases, not receiving copies from some critical neighbors may be due to that and not to them not receiving the message. thus, retransmiting on these phases is a very conservative approach to avoid message delivery failures.

        if(!all_neighs_covered) {
            double missed_critical_neighs = getMissedCriticalNeighs(p_msg, myID, contexts);

            if(missed_critical_neighs > args->min_critical_coverage)
                return true;
        }
    }

    return false;
}

static void CriticalNeighPolicyDestroy(ModuleState* policy_state) {
    free(policy_state->args);
}

RetransmissionPolicy* CriticalNeighPolicy(bool all_copies, bool mobility_extension, double min_critical_coverage) {
    assert( 0.0 <= min_critical_coverage && min_critical_coverage  <= 1.0 );

    CriticalNeighPolicyArgs* args = malloc(sizeof(CriticalNeighPolicyArgs));
    args->all_copies = all_copies;
    args->mobility_extension = mobility_extension;
    args->min_critical_coverage = min_critical_coverage;

    RetransmissionPolicy* r_policy = newRetransmissionPolicy(
        args,
        NULL,
        &CriticalNeighPolicyEval,
        &CriticalNeighPolicyDestroy,
        new_list(1, new_str("LabelNeighsContext"))
    );

    return r_policy;
}
