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

static bool DynamicProbabilityPolicyEval(ModuleState* policy_state, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts) {

    RetransmissionContext* r_context = hash_table_find_value(contexts, "DynamicProbabilityContext");
    assert(r_context);

    double p = 0.0;
    if(!RC_query(r_context, "p", &p, NULL, myID, contexts))
        assert(false);

    //printf("P = %f\n", p);
    /*
char str[20];
    sprintf(str, "p=%f", p);
    my_logger_write(broadcast_logger, "DYNAMIC_PROBABILITY", "", str);
*/

	double u = randomProb();
	return u <= p;
}

RetransmissionPolicy* DynamicProbabilityPolicy() {

    RetransmissionPolicy* r_policy = newRetransmissionPolicy(
        NULL,
        NULL,
        &DynamicProbabilityPolicyEval,
        NULL,
        new_list(1, new_str("DynamicProbabilityContext"))
    );

    return r_policy;
}
