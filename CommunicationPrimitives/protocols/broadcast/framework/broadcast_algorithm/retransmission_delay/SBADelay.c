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

static unsigned long _SBADelay(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, RetransmissionContext* r_context, unsigned char* myID) {
    if(!isCopy) {
        unsigned long t = *((unsigned long*) (delay_state->args));

        unsigned int n, max_n;
        if(!query_context(r_context, "n_neighbors", &n, myID, 0))
            assert(false);
        if(!query_context(r_context, "max_neighbors", &max_n, myID, 0))
            assert(false);

        return getDelay(t, n, max_n);
    } else {
        return remaining;
    }
}

static void _SBADelayDestroy(ModuleState* delay_state, list* visited) {
    free(delay_state->args);
}

RetransmissionDelay* SBADelay(unsigned long t) {
    RetransmissionDelay* r_delay = malloc(sizeof(RetransmissionDelay));

	r_delay->delay_state.args = malloc(sizeof(t));
	*((unsigned long*)(r_delay->delay_state.args)) = t;

	r_delay->delay_state.vars = NULL;
	r_delay->r_delay = &_SBADelay;
    r_delay->destroy = &_SBADelayDestroy;

	return r_delay;
}
