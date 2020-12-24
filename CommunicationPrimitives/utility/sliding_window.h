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

#ifndef SLIDING_WINDOW_H_
#define SLIDING_WINDOW_H_

#include "Yggdrasil.h"

typedef struct SlidingWindow_ SlidingWindow;

SlidingWindow* newSlidingWindow(unsigned int size);

unsigned int SW_getSize(SlidingWindow* s);

void SW_pushValue(SlidingWindow* s, bool value);

unsigned int SW_compute(SlidingWindow* s);

#endif /*SLIDING_WINDOW_H_*/
