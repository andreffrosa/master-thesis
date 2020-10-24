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

#include <assert.h>

typedef struct _Window {
    list* w;
    unsigned int n_buckets;
    unsigned int bucket_duration_s;
    struct timespec start_time;
} Window;

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

    while(w->head != NULL) {
        Tuple* x = (Tuple*)w->head->data;
        struct timespec* t = (struct timespec*)(x->entries[0]);
        if(compare_timespec(t, window_start) < 0) {
            x = list_remove_head(w);

            free(x->entries[0]);
            free(x->entries[1]);
            free(x->entries);
            free(x);
        } else {
            break;
        }
    }

}

void insertIntoWindow(Window* w, struct timespec* t, double value) {
    assert(w);

    bool valid = w->w->size > 0 ? compare_timespec(t, ((struct timespec*)(((Tuple*)(w->w->tail->data))->entries[0]))) >= 0 : true;
    assert(valid);

    if( timespec_is_zero(&w->start_time) ) {
        copy_timespec(&w->start_time, t);
    } else {
        unsigned int window_duration_s = w->n_buckets * w->bucket_duration_s;

        struct timespec window_duration, window_start;
        milli_to_timespec(&window_duration, window_duration_s*1000);
        subtract_timespec(&window_start, t, &window_duration);

        windowGC(w, &window_start);
    }

    Tuple* x = newTuple(malloc(2*sizeof(void*)), 2);
    x->entries[0] = malloc(sizeof(struct timespec));
    copy_timespec(x->entries[0], t);
    x->entries[1] = malloc(sizeof(double));
    *((double*)x->entries[1]) = value;

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
        struct timespec* tail = (struct timespec*)(((Tuple*)w->w->tail->data)->entries[0]);
        struct timespec aux;
        subtract_timespec(&aux, tail, &w->start_time);

        n_buckets = (timespec_to_milli(&aux)/1000) / w->bucket_duration_s;
        n_buckets = (n_buckets == 0 ? 1 : n_buckets);

        copy_timespec(&window_start, &w->start_time);
    }
    assert(n_buckets > 0);

    double buckets[n_buckets];
    unsigned int amount[n_buckets];
    memset(buckets, 0, sizeof(buckets));
    memset(amount, 0, sizeof(amount));

    for(list_item* it = w->w->head; it; it = it->next) {
        Tuple* x = (Tuple*)it->data;
        struct timespec* t = (struct timespec*)x->entries[0];
        double value = *( (double*)(x->entries[1]) );

        // Compute bucket
        struct timespec aux;
        subtract_timespec(&aux, t, &window_start);
        unsigned int bucket = ((unsigned int)(timespec_to_milli(&aux) / 1000)) / w->bucket_duration_s; // integer division
        bucket = (bucket == n_buckets ? bucket-1 : bucket);

        buckets[bucket] += value;
        amount[bucket]++;
    }

    if( strcmp(bucket_type, "avg") == 0 ) {
        for(int i = 0; i < n_buckets; i++) {
            buckets[i] = buckets[i] / amount[i];
        }
    }

    if( per_second ) {
        for(int i = 0; i < n_buckets; i++) {
            buckets[i] /= w->bucket_duration_s;
        }
    }

    return compute_moving_avg(buckets, n_buckets, window_type);
}
