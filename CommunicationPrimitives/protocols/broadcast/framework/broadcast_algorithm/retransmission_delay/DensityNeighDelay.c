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

#include "retransmission_delay_private.h"

#include <math.h>
#include <assert.h>

#include "utility/my_math.h"

static unsigned long getCurrentPhaseDelay(unsigned long t_max, unsigned int n, double p, double cdf) {
	double u = getRandomProb();
    unsigned long delay = 0L;
	if( n == 0 ) {
		delay = (unsigned long) roundl(u*t_max);
	} else if( n <= 2 ) { // Only one more neigh
		delay = (unsigned long) roundl((u/10.0)*t_max); // Just to avoid hidden terminal collisions
	} else {
		double t_factor = cdf - p*u;
		double _interval = ( dMin(log( n/2.0 ), 1.0) * t_max );
		delay = (unsigned long) roundl( t_factor * _interval );
	}
    assert(delay <= t_max);
    return delay;
}

static unsigned long _DensityNeighDelay(ModuleState* delay_state, PendingMessage* p_msg, unsigned long remaining, bool isCopy, RetransmissionContext* r_context, unsigned char* myID) {

    if(!isCopy) {
        unsigned long t_max = *((unsigned long*) (delay_state->args));

        double aux[3];
        if(!query_context(r_context, "neighbors_distribution", aux, myID, 0))
            assert(false);

        unsigned int current_phase = getCurrentPhase(p_msg);
        //unsigned long previous_phase_remaining = current_phase == 1 ? 0: t_max - getPhaseDuration(getPhaseStats(p_msg, current_phase-1));

        //return previous_phase_remaining + getCurrentPhaseDelay(t_max, (int)aux[0], aux[1], aux[2]);
        return (current_phase == 1 ? 0 : t_max) + getCurrentPhaseDelay(t_max, (int)aux[0], aux[1], aux[2]);
    }
    else {
        return remaining;
    }
}

static void _DensityNeighDelayDestroy(ModuleState* delay_state, list* visited) {
    free(delay_state->args);
}

RetransmissionDelay* DensityNeighDelay(unsigned long t_max) {
	RetransmissionDelay* r_delay = malloc(sizeof(RetransmissionDelay));

	r_delay->delay_state.args = malloc(sizeof(t_max));
	*((unsigned long*)(r_delay->delay_state.args)) = t_max;

	r_delay->delay_state.vars = NULL;
	r_delay->r_delay = &_DensityNeighDelay;
    r_delay->destroy = &_DensityNeighDelayDestroy;

	return r_delay;
}
