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

#include "link_quality_private.h"

#include "utility/sliding_window.h"

#include <math.h>
#include <assert.h>

typedef struct _SWLQArgs {
    double initial_quality;
    unsigned int window_size;
} SWLQArgs;

static void* create_attrs(ModuleState* state) {
    SWLQArgs* args = (SWLQArgs*)state->args;
    return newSlidingWindow(args->window_size);
}

static void destroy_attrs(ModuleState* state, void* lq_attrs) {
    free((SlidingWindow*)lq_attrs);
}

static double compute_quality(ModuleState* state, void* lq_attrs, double previous_link_quality, unsigned int received, unsigned int lost, bool init, struct timespec* current_time) {
    SWLQArgs* args = (SWLQArgs*)state->args;
    SlidingWindow* window = (SlidingWindow*)lq_attrs;

    if( init ) {
        // Insert into window

        unsigned int n = floor(args->initial_quality * SW_getSize(window));
        for(int i = 0; i < n; i++) {
            SW_pushValue(window, true);
        }

        // Debug
        //double initial_quality = ((double)SW_compute(window)) / SW_getSize(window);
        //printf("\n\n\t\tComputed Initial Quality: %f\n", initial_quality);

        return args->initial_quality;
    } else {
        for(int i = 0; i < lost; i++) {
            SW_pushValue(window, false);
        }
        for(int i = 0; i < received; i++) {
            SW_pushValue(window, true);
        }

        // Compute current link quality
        double quality = ((double)SW_compute(window)) / SW_getSize(window);

        //printf("Computed Quality: %f\n", quality);
        //printf("\n\n\t\tCompute LQ: lost=%d received=%d old_lq=%f new_lq=%f\n\n", lost, received, previous_link_quality, quality);

        return quality;
    }
}

static void destroy(ModuleState* lq_state) {
    free(lq_state->args);
}

LinkQuality* SlidingWindowLinkQuality(double initial_quality, unsigned int window_size) {
    assert(0 < window_size);
    assert(0.0 <= initial_quality && initial_quality <= 1.0);

    SWLQArgs* args = malloc(sizeof(SWLQArgs));
    args->initial_quality = initial_quality;
    args->window_size = window_size;

    return newLinkQualityMetric(args, NULL, &compute_quality, &create_attrs, &destroy_attrs, &destroy);
}
