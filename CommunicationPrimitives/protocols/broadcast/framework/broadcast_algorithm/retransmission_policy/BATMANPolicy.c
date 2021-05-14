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

static bool BATMANPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {

    unsigned int current_phase = getCurrentPhase(p_msg);

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

        bool result = false;

        if(bi) {
            r_context = hash_table_find_value(contexts, "BATMANContext");
            assert(r_context);

            double_list* copies = getCopies(p_msg);

            uuid_t best_neigh;
            found = RC_query(r_context, "best_neigh", best_neigh, query_args, myID, contexts);
            if(found) {
                char str[UUID_STR_LEN];
                uuid_unparse(best_neigh, str);
                printf("best_neigh = %s\n", str);

                for(double_list_item* it = copies->head; it; it = it->next) {
                    MessageCopy* copy = (MessageCopy*)it->data;
                    unsigned char* parent_id = getBcastHeader(copy)->sender_id;
                    unsigned int phase = getCopyPhase(copy);

                    if(uuid_compare(parent_id, best_neigh) == 0) {
                        if(phase == current_phase) {
                            if(phase == 1) {
                                result = true;
                                break;
                            } else {
                                MessageCopy* first_copy = (MessageCopy*)copies->head->data;

                                byte* fisrt_hops_ = (byte*)hash_table_find_value(getHeaders(first_copy), "hops");
                                byte* current_hops_ = (byte*)hash_table_find_value(getHeaders(copy), "hops");

                                if(fisrt_hops_ && current_hops_) {
                                    if(*fisrt_hops_ == *current_hops_) {
                                        result = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            else {
                unsigned char* src_id = getBcastHeader(copies->head->data)->source_id;

                if(uuid_compare(src_id, myID) == 0) {
                    result = false;
                } else {
                    char str[UUID_STR_LEN];
                    uuid_unparse(src_id, str);
                    printf("=> best_neigh not found! for src=%s\n", str);

                    result = true;
                }
            }
        }

        hash_table_delete(query_args);

        return result;
}

RetransmissionPolicy* BATMANPolicy() {
    return newRetransmissionPolicy(
        NULL,
        NULL,
        &BATMANPolicyEval,
        NULL,
        new_list(2, new_str("BATMANContext"), new_str("BiFloodingContext"))
    );

}
