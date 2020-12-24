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

#include "sliding_window.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct SlidingWindow_ {
    unsigned int size;
    bool window[];
} SlidingWindow;

SlidingWindow* newSlidingWindow(unsigned int size) {
    SlidingWindow* s = (SlidingWindow*)malloc(sizeof(SlidingWindow) + size*(sizeof(bool)));

    s->size = size;
    memset(s->window, false, size*sizeof(bool));

    return s;
}

unsigned int SW_getSize(SlidingWindow* s) {
    assert(s);
    return s->size;
}

void SW_pushValue(SlidingWindow* s, bool value) {
    assert(s);

    bool aux[s->size];
    memcpy(aux, s->window + 1, (s->size-1)*sizeof(bool));
    aux[s->size-1] = value;
    memcpy(s->window, aux, s->size*sizeof(bool));
}

unsigned int SW_compute(SlidingWindow* s) {
    assert(s);

    unsigned int counter = 0;
    for(unsigned int i = 0; i < s->size; i++) {
        counter += (s->window[i] ? 1 : 0);
    }

    return counter;
}
