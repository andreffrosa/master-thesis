/*********************************************************
 * This code was written in the r_policy of the Lightkone
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

void destroyRetransmissionPolicy(RetransmissionPolicy* r_policy, list* visited) {
     if(r_policy !=NULL) {
         if(r_policy->destroy != NULL) {
             bool root = visited == NULL;
             if(root) {
                 visited = list_init();
                 void** this = malloc(sizeof(void*));
                 *this = r_policy;
                 list_add_item_to_tail(visited, this);
             }

             r_policy->destroy(&r_policy->policy_state, visited);

             if(root) {
                 list_delete(visited);
             }
         }
         free(r_policy);
     }
}
