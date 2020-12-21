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

#include "utility/my_math.h"

#include <assert.h>

static bool DynamicProbabilityPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, RetransmissionContext* r_context, list* visited) {
	double p = 0.0;

    list* visited2 = list_init();
    if(!RC_query(r_context, "p", &p, NULL, myID, visited2))
        assert(false);
    list_delete(visited2);

    //printf("P = %f\n", p);
    char str[20];
    sprintf(str, "p=%f", p);
    ygg_log("DYNAMIC_PROBABILITY", "", str);

	double u = randomProb();
	return u <= p;
}

RetransmissionPolicy* DynamicProbabilityPolicy() {
	return newRetransmissionPolicy(
        NULL,
        NULL,
        &DynamicProbabilityPolicyEval,
        NULL
    );
}
