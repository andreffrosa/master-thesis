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

#include "window.h"

#include "data_structures/list.h"

#include "utility/my_time.h"
#include "utility/my_misc.h"
#include "utility/my_math.h"

#include <assert.h>

typedef struct Window_ {
    list* w;
    unsigned int n_buckets;
    unsigned int bucket_duration_s;
    struct timespec start_time;
} Window;

typedef struct WindowEntry_ {
    struct timespec t;
    double v;
} WindowEntry;

Window* newWindow(unsigned int n_buckets, unsigned int bucket_duration_s) {
    Window* w = malloc(sizeof(Window));

    w->w = list_init();
    w->n_buckets = n_buckets;
    w->bucket_duration_s = bucket_duration_s;
    copy_timespec(&w->start_time, &zero_timespec);

    return w;
}

void destroyWindow(Window* window) {
    if(window) {
        list_delete(window->w);
        free(window);
    }
}

static void windowGC(Window* window, struct timespec* window_start) {

    list* w = window->w;

    bool stop = false;
    while(!stop) {
        if( w->head ) {
            struct timespec* t = &((WindowEntry*)w->head->data)->t;

            if(compare_timespec(t, window_start) <= 0) {
                WindowEntry* x = list_remove_head(w);
                free(x);
            } else {
                stop = true;
            }
        } else {
            stop = true;
        }
    }

    /* if(w->size > 0) {
        struct timespec* first = &((WindowEntry*)w->head->data)->t;
        assert(compare_timespec(first, window_start) > 0);
    } */

}

void insertIntoWindow(Window* w, struct timespec* t, double value) {
    assert(w);

    if(w->w->size > 0) {
        struct timespec* last = &((WindowEntry*)w->w->tail->data)->t;
        bool valid = compare_timespec(t, last) >= 0;
        assert(valid);

        /* unsigned int window_duration_s = w->n_buckets * w->bucket_duration_s;

        struct timespec window_duration, window_start;
        milli_to_timespec(&window_duration, window_duration_s*1000);
        subtract_timespec(&window_start, t, &window_duration);

        windowGC(w, &window_start); */
    } else {
        if( timespec_is_zero(&w->start_time) ) {
            copy_timespec(&w->start_time, t);
        }
    }

    WindowEntry* x = malloc(sizeof(WindowEntry));
    copy_timespec(&x->t, t);
    x->v = value;

    list_add_item_to_tail(w->w, x);
}

static double compute_moving_avg(double* buckets, unsigned int n_buckets, char* window_type) {
    char aux[strlen(window_type)+1];
    strcpy(aux, window_type);
    char* ptr = NULL;
    char* token  = strtok_r(aux, " ", &ptr);

    // Simple Moving Average
    if(strcmp(token, "avg") == 0 || strcmp(token, "sma") == 0) {
        double sma = 0.0;
        for(int i = 0; i < n_buckets; i++) {
            sma += buckets[i];
        }
        sma /= n_buckets;

        return sma;
    }

    // Weighted Moving Average
    else if(strcmp(token, "wma") == 0) {
        double wma = 0.0;
        for(int i = 0; i < n_buckets; i++) {
            wma += (i+1)*buckets[i];
        }
        wma /= (n_buckets*(n_buckets + 1)) / 2.0;

        return wma;
    }

    // Exponetially Weighted Moving Average
    else if(strcmp(token, "ema") == 0 || strcmp(token, "ewma") == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double alfa = strtod(token, NULL);
            assert(0.0 < alfa && alfa < 1.0);

            double ema = buckets[0];
            for(int i = 1; i < n_buckets; i++) {
                ema = alfa*buckets[i] + (1.0-alfa)*ema;
            }

            return ema;
        } else {
            printf("Parameter 1 of %s not passed!\n", "ema");
            exit(-1);
        }
    }

    else {
        printf("Unknown window type!\n");
        exit(-1);
    }
}

double computeWindow(Window* w, struct timespec* current_time, char* window_type, char* bucket_type, bool per_second) {
    assert(w);

    unsigned int window_duration_s = w->n_buckets * w->bucket_duration_s;

    struct timespec window_duration, window_start;
    milli_to_timespec(&window_duration, window_duration_s*1000);
    subtract_timespec(&window_start, current_time, &window_duration);

    windowGC(w, &window_start);

    if(w->w->size == 0) {
        return 0.0;
    }

    // Split into buckets
    unsigned int n_buckets = w->n_buckets;
    if( compare_timespec(&w->start_time, &window_start) > 0 ) {

        struct timespec elapsed_t;
        subtract_timespec(&elapsed_t, current_time, &w->start_time);
        unsigned int elapsed_s = (unsigned int)(timespec_to_milli(&elapsed_t) / 1000);

        // the diff between this entry and start_time is smaller than 1ms and thus is not deleted by the GC yet it appears as equal in the ms and above
        assert(elapsed_s <= window_duration_s);

        n_buckets = iMin((elapsed_s / w->bucket_duration_s) + 1, w->n_buckets);
    }
    assert(n_buckets > 0 && n_buckets <= w->n_buckets);

    double buckets[n_buckets];
    unsigned int amount[n_buckets];
    memset(buckets, 0, sizeof(buckets));
    memset(amount, 0, sizeof(amount));

    // Compute each bucket
    for(list_item* it = w->w->head; it; it = it->next) {
        WindowEntry* entry = (WindowEntry*)it->data;

        //assert(compare_timespec(&entry->t, &window_start) > 0);

        //assert(compare_timespec(&entry->t, current_time) <= 0);

        // Compute bucket
        struct timespec elapsed_t;
        subtract_timespec(&elapsed_t, current_time, &entry->t);
        unsigned int elapsed_s = (unsigned int)(timespec_to_milli(&elapsed_t) / 1000);

        //assert(elapsed_s < window_duration_s);

        // To avoid when the diff between this entry and start_time is smaller than 1ms and thus is not deleted by the GC yet it appears as equal in the ms and above
        unsigned int bucket = (elapsed_s == window_duration_s) ? 0 : ((n_buckets-1) - (elapsed_s / w->bucket_duration_s)); // integer division
        //bucket = (bucket == n_buckets ? bucket-1 : bucket);
        //assert(bucket < n_buckets);
        /* if(bucket >= n_buckets) {
            printf("current_time={%lu %lu} entry={%lu %lu} window_start={%lu %lu}\n", current_time->tv_sec, current_time->tv_nsec, entry->t.tv_sec, entry->t.tv_nsec, window_start.tv_sec, window_start.tv_nsec);
            printf("elapsed_s = %u w->bucket_duration_s = %u window_duration_s = %u bucket = %u n_buckets = %u\n", elapsed_s, w->bucket_duration_s, window_duration_s, bucket, n_buckets);
            fflush(stdout);
            assert(bucket < n_buckets);
        }*/
        assert(bucket < n_buckets);

        //printf("value=%f to bucket %d\n", value, bucket);

        buckets[bucket] += entry->v;
        amount[bucket]++;
    }

    if( strcmp(bucket_type, "avg") == 0 ) {
        for(int i = 0; i < n_buckets; i++) {
            buckets[i] = amount[i] == 0 ? 0.0 : buckets[i] / amount[i];
        }
    }

    if( per_second ) {
        for(int i = 0; i < n_buckets; i++) {
            buckets[i] /= w->bucket_duration_s;
        }
    }

    //#if DEBUG_INCLUDE_GT(DISCOVERY_DEBUG_LEVEL, ADVANCED_DEBUG)
    /*char str[200];
    sprintf(str, "[%.3f", buckets[0]);
    char* ptr = str + strlen(str);
    for(int i = 1; i < n_buckets; i++) {
        sprintf(ptr, ", %.3f", buckets[i]);
        ptr+=strlen(ptr);
    }
    strcpy(ptr, "]");
    ygg_log("WINDOW", "BUCKETS", str);*/
    //#endif

double result = compute_moving_avg(buckets, n_buckets, window_type);

return (result == NAN || result == -NAN) ? 0.0 : result;
}

unsigned int getWindowNBuckets(Window* w) {
    assert(w);
    return w->n_buckets;
}

unsigned int getWindowBucketDurationS(Window* w) {
    assert(w);
    return w->bucket_duration_s;
}
