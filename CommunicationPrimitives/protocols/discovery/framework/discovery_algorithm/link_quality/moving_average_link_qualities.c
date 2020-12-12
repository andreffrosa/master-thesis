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

#include <assert.h>

typedef struct _MALQArgs {
    double initial_quality;
    char window_type[20];
    unsigned int bucket_duration_s;
    unsigned int n_buckets;
} MALQArgs;

static void* create_attrs(ModuleState* state) {
    MALQArgs* args = (MALQArgs*)state->args;
    return newWindow(args->n_buckets, args->bucket_duration_s);
}

static void destroy_attrs(ModuleState* state, void* lq_attrs) {
    destroyWindow((Window*)lq_attrs);
}

static double compute_quality(ModuleState* state, void* lq_attrs, double previous_link_quality, unsigned int received, unsigned int lost, bool init, struct timespec* current_time) {
    MALQArgs* args = (MALQArgs*)state->args;
    Window* window = (Window*)lq_attrs;

    if( init ) {
        // Insert into window
        /*
        int samples = 25;
        unsigned int x = samples*args->initial_quality;
        for(int i = 0; i < x; i++) {
            insertIntoWindow(window, current_time, 1.0);
        }
        for(int i = x; i < samples; i++) {
            insertIntoWindow(window, current_time, 0.0);
        }
        */
        struct timespec moment = {0};
        milli_to_timespec(&moment, getWindowBucketDurationS(window)*1000);
        subtract_timespec(&moment, current_time, &moment);
        insertIntoWindow(window, &moment, args->initial_quality);

        // Debug
        // double initial_quality = computeWindow(window, current_time, args->window_type, "avg", false);
        // printf("Computed Initial Quality: %f\n", initial_quality);

        return args->initial_quality;
    } else {
        for(int i = 0; i < lost; i++) {
            insertIntoWindow(window, current_time, 0.0);
        }
        for(int i = 0; i < received; i++) {
            insertIntoWindow(window, current_time, 1.0);
        }

        // Compute current link quality
        double quality = computeWindow(window, current_time, args->window_type, "avg", false);

        // printf("Computed Quality: %f\n", quality);

        //printf("\n\n\t\tCompute LQ: lost=%d received=%d old_lq=%f new_lq=%f\n\n", lost, received, previous_link_quality, quality);

        return quality;
    }
}

static void destroy(ModuleState* lq_state/*, list* visited*/) {
    free(lq_state->args);
}

static LinkQuality* MovingAverageLQ(double initial_quality, unsigned int n_buckets, unsigned int bucket_duration_s, char* window_type) {
    assert(0.0 <= initial_quality && initial_quality <= 1.0);

    MALQArgs* args = malloc(sizeof(MALQArgs));
    args->initial_quality = initial_quality;
    strcpy(args->window_type, window_type);
    args->n_buckets = n_buckets;
    args->bucket_duration_s = bucket_duration_s;

    return newLinkQualityMetric(args, NULL, &compute_quality, &create_attrs, &destroy_attrs, &destroy);
}

LinkQuality* SMALinkQuality(double initial_quality,  unsigned int n_buckets, unsigned int bucket_duration_s) {
    return MovingAverageLQ(initial_quality, n_buckets, bucket_duration_s, "sma");
}

LinkQuality* WMALinkQuality(double initial_quality,  unsigned int n_buckets, unsigned int bucket_duration_s) {
    return MovingAverageLQ(initial_quality, n_buckets, bucket_duration_s, "wma");
}

LinkQuality* EMALinkQuality(double initial_quality, double scalling,  unsigned int n_buckets, unsigned int bucket_duration_s) {
    assert(0.0 < scalling && scalling < 1.0);

    char window_type[20];
    sprintf(window_type, "ema %f", scalling);

    return MovingAverageLQ(initial_quality, n_buckets, bucket_duration_s, window_type);
}
