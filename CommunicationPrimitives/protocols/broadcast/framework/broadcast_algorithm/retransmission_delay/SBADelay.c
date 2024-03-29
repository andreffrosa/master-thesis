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

#include "retransmission_delay_private.h"

#include <math.h>
#include <assert.h>

#include "utility/my_math.h"

// FROM "W. Peng and X. Lu. On the reduction of broadcast redundancy in mobile ad hoc networks. In Proceedings of MOBIHOC, 2000"

static unsigned long getDelay(unsigned long t, unsigned int n, unsigned int max_n) {
	double u = randomProb();
	double t0 = (1.0 + max_n) / (1.0 + n);
	return (unsigned long) roundl( t*t0*u );
}

static unsigned long SBADelayCompute(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID, hash_table* contexts) {
    if(!isCopy) {
        unsigned long t = *((unsigned long*) (delay_state->args));

        RetransmissionContext* r_context = hash_table_find_value(contexts, "NeighborsContext");
        assert(r_context);

        unsigned int n, max_n;
        if(!RC_query(r_context, "n_neighbors", &n, NULL, myID, contexts))
            assert(false);


        if(!RC_query(r_context, "max_neighbors", &max_n, NULL, myID, contexts))
            assert(false);

        return getDelay(t, n, max_n);
    } else {
        return remaining;
    }
}

static void SBADelayDestroy(ModuleState* delay_state) {
    free(delay_state->args);
}

RetransmissionDelay* SBADelay(unsigned long t) {

    unsigned long* t_arg = malloc(sizeof(t));
	*t_arg = t;

    RetransmissionDelay* r_delay = newRetransmissionDelay(
        t_arg,
        NULL,
        &SBADelayCompute,
        &SBADelayDestroy,
        new_list(1, new_str("NeighborsContext"))
    );

    return r_delay;
}
