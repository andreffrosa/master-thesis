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

#ifndef _UTILITY_SEQ_H_
#define _UTILITY_SEQ_H_

#include <limits.h>

#include "Yggdrasil.h"

#define MAX_SEQ (USHRT_MAX+1)
#define MAX_SEQ_HALF (MAX_SEQ / 2)

unsigned short inc_seq(unsigned short current_seq, bool ignore_zero);

int compare_seq(unsigned short s1, unsigned short s2, bool ignore_zero);

#endif /* _UTILITY_SEQ_H_ */
