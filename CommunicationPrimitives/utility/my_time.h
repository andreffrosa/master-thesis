
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

#ifndef MY_TIME_H_
#define MY_TIME_H_

#include <uuid/uuid.h>

#include "Yggdrasil/core/protos/timer.h"
#include "Yggdrasil/core/utils/utils.h"

// Timespec
extern const struct timespec zero_timespec;

void copy_timespec(const struct timespec* a, const struct timespec* b);

char* timespec_to_string(struct timespec* t, char* str, int len, int precision);
int subtract_timespec(struct timespec *result, struct timespec *x, struct timespec *y);
void add_timespec(struct timespec *result, struct timespec *x, struct timespec *y);
//int compare_timespec(struct timespec* a, struct timespec* b);
void clear_timespec(struct timespec* t);
int timespec_is_zero(struct timespec* t);
void multiply_timespec(struct timespec* t, struct timespec* time, double alfa);
void random_timespec(struct timespec* t, struct timespec* base, struct timespec* interval);
void milli_to_timespec(struct timespec *result, unsigned long milli );
unsigned long timespec_to_milli(struct timespec* time);
unsigned long random_mili_to_timespec(struct timespec* t, unsigned long max_jitter);

// Timer
void SetTimer(struct timespec* t, unsigned char* id, unsigned short protoID, short int type);
void SetTimerWithPayload(struct timespec* t, unsigned char* id, unsigned short protoID, short int type, unsigned char* payload, unsigned short payload_size);
void SetPeriodicTimer(struct timespec* t, unsigned char* id, unsigned short protoID, short int type);
void CancelTimer(uuid_t id, unsigned short protoID);

#endif /* MY_TIME_H_ */
