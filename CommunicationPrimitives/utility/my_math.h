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

#ifndef MY_MATH_H_
#define MY_MATH_H_

#include "Yggdrasil/core/utils/utils.h"

// Min and Max
long lMin(long a, long b);
long lMax(long a, long b);
double dMin(double a, double b);
double dMax(double a, double b);
int iMin(int a, int b);
int iMax(int a, int b);

bool isPrime(unsigned int n);
unsigned int nextPrime(unsigned int N);

int compare_int(int a, int b);

// Random
int randomInt(int min, int max);
long randomLong();
double randomProb();
double randomExponential(double lambda);

#endif /* MY_MATH_H_ */
