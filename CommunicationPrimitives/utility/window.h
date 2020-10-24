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

#ifndef _UTILITY_WINDOW_H_
#define _UTILITY_WINDOW_H_

#include <time.h>

#include "utility/my_time.h"

typedef struct _Window Window;

Window* newWindow(unsigned int n_buckets, unsigned int bucket_duration_s);

void destroyWindow(Window* window);

void insertIntoWindow(Window* w, struct timespec* t, double value);

// type = avg or sma, wma, ema <alfa>
double computeWindow(Window* w, struct timespec* current_time, char* window_type, char* bucket_type, bool per_second);

#endif /* _UTILITY_WINDOW_H_ */
