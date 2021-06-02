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

#include "my_math.h"

#include <math.h>
#include <assert.h>

double dMin(double a, double b) {
	return a < b ? a : b;
}

double dMax(double a, double b) {
	return a > b ? a : b;
}

long lMin(long a, long b) {
	return a < b ? a : b;
}

long lMax(long a, long b) {
	return a > b ? a : b;
}

unsigned long ulMin(unsigned long a, unsigned long b) {
	return a < b ? a : b;
}

unsigned long ulMax(unsigned long a, unsigned long b) {
	return a > b ? a : b;
}

int iMin(int a, int b) {
	return a < b ? a : b;
}

int iMax(int a, int b) {
	return a > b ? a : b;
}

int compare_int(int a, int b) {
	if(a == b)
		return 0;
	else if(a < b)
		return -1;
	else
		return 1;
}

long randomLong() {
	int n = ((sizeof(long)/2)*8) - 1;
	long r = rand();
	r = (r << n) | rand();

	r = labs(r);

	return r;
}



// From https://www.geeksforgeeks.org/program-to-find-the-next-prime-number/
bool isPrime(unsigned int n) {
	if (n <= 1) return false;
	if (n <= 3) return true;

	// This is checked so that we can skip
	// middle five numbers in below loop
	if (n%2 == 0 || n%3 == 0) return false;

	for (int i=5; i*i<=n; i=i+6)
		if (n%i == 0 || n%(i+2) == 0)
			return false;

	return true;
}

unsigned int nextPrime(unsigned int N) {

	// Base case
	if (N <= 1)
		return 2;

	unsigned int prime = N;
	bool found = false;

	// Loop continuously until isPrime returns
	// true for a number greater than n
	while (!found) {
		prime++;

		if (isPrime(prime))
			found = true;
	}

	return prime;
}

// Both min and max are included in the possible values
int randomInt(int min, int max) {
	return (rand() % (max - min + 1)) + min;
}

double randomProb() {
	return (double)rand() / RAND_MAX;
}

double randomExponential(double lambda) {
    if(lambda < 0) {
        return 0;
    } else {
        double u = randomProb();
        //double exp = u < 0.01 ? 0.01 : -1.0*log(u) / lambda;
        double exp = u == 0 ? 0.0 : (-1.0*log(u) / lambda);
        return exp;
    }
}


double roundPrecision(double value, int precision) {

    /* if(precision == 0) {
        return value;
    } else { */
        double x = pow(10, precision);
        return round(value/x)*x;
    //}
}
