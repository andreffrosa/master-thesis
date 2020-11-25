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

#ifndef _DISCOVERY_ENVIRONMENT_H_
#define _DISCOVERY_ENVIRONMENT_H_

#include "Yggdrasil.h"

typedef struct DiscoveryEnvironment_ DiscoveryEnvironment;

DiscoveryEnvironment* newDiscoveryEnvironment(unsigned int traffic_n_bucket, unsigned int traffic_bucket_duration_s, unsigned int churn_n_bucket, unsigned int churn_bucket_duration_s);

void deleteDiscoveryEnvironment(DiscoveryEnvironment* de);

void DE_registerOutTraffic(DiscoveryEnvironment* de, struct timespec* current_time);

bool DE_computeOutTraffic(DiscoveryEnvironment* de, struct timespec* current_time, char* window_type, double epsilon);

void DE_registerNewNeighbor(DiscoveryEnvironment* de, struct timespec* current_time);

bool DE_computeNewNeighborsFlux(DiscoveryEnvironment* de, struct timespec* current_time, char* window_type, double epsilon);

void DE_registerLostNeighbor(DiscoveryEnvironment* de, struct timespec* current_time);

bool DE_computeLostNeighborsFlux(DiscoveryEnvironment* de, struct timespec* current_time, char* window_type, double epsilon);

double DE_getOutTraffic(DiscoveryEnvironment* de);

double DE_getNewNeighborsFlux(DiscoveryEnvironment* de);

double DE_getLostNeighborsFlux(DiscoveryEnvironment* de);

double DE_getInTraffic(DiscoveryEnvironment* de);

bool DE_setInTraffic(DiscoveryEnvironment* de, double new_in_traffic, double epsilon);

unsigned int DE_getNNeighbors(DiscoveryEnvironment* de);

bool DE_setNNeighbors(DiscoveryEnvironment* de, unsigned int new_n_neighbors);

bool DE_setNeigbhorsDensity(DiscoveryEnvironment* de, double new_neighbors_density, double epsilon);

double DE_getNeigbhorsDensity(DiscoveryEnvironment* de);

char* NE_print(DiscoveryEnvironment* de, char** str);

#endif /* _DISCOVERY_ENVIRONMENT_H_ */
