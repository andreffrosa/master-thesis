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

#include "discovery_environment.h"

#include "utility/window.h"
#include "utility/my_math.h"

#include <assert.h>
#include <math.h>

typedef struct DiscoveryEnvironment_ {
    Window* out_traffic_window;
    Window* new_neighbors_flux_window;
    Window* lost_neighbors_flux_window;

    double old_in_traffic;
    double old_out_traffic;
    double old_new_neighbors_flux;
    double old_lost_neighbors_flux;

    unsigned int old_n_neighbors;
    double old_neighbors_density;
} DiscoveryEnvironment;

DiscoveryEnvironment* newDiscoveryEnvironment(unsigned int traffic_n_bucket, unsigned int traffic_bucket_duration_s, unsigned int churn_n_bucket, unsigned int churn_bucket_duration_s) {
    DiscoveryEnvironment* de = malloc(sizeof(DiscoveryEnvironment));

    de->out_traffic_window = newWindow(traffic_n_bucket, traffic_bucket_duration_s);
    de->new_neighbors_flux_window = newWindow(churn_n_bucket, churn_bucket_duration_s);
    de->lost_neighbors_flux_window = newWindow(churn_n_bucket, churn_bucket_duration_s);

    de->old_in_traffic = 0.0;
    de->old_out_traffic = 0.0;
    de->old_new_neighbors_flux = 0.0;
    de->old_lost_neighbors_flux = 0.0;
    de->old_n_neighbors = 0;
    de->old_neighbors_density = 0.0;

    return de;
}

void deleteDiscoveryEnvironment(DiscoveryEnvironment* de) {
    if(de) {
        destroyWindow(de->out_traffic_window);
        destroyWindow(de->new_neighbors_flux_window);
        destroyWindow(de->lost_neighbors_flux_window);
    }
}

void DE_registerOutTraffic(DiscoveryEnvironment* de, struct timespec* current_time) {
    assert(de);
    insertIntoWindow(de->out_traffic_window, current_time, 1.0);
}

bool DE_computeOutTraffic(DiscoveryEnvironment* de, struct timespec* current_time, char* window_type, double epsilon, int precision) {
    assert(de);
    double new_out_traffic = computeWindow(de->out_traffic_window, current_time, window_type, "sum", true);
    new_out_traffic = roundPrecision(new_out_traffic, precision);

    double delta = fabs(new_out_traffic - de->old_out_traffic);

    if( delta >= epsilon || (delta > 0 && (new_out_traffic == 0.0)) ) {
        de->old_out_traffic = new_out_traffic;
        return true;
    }
    return false;
}

void DE_registerNewNeighbor(DiscoveryEnvironment* de, struct timespec* current_time) {
    assert(de);
    insertIntoWindow(de->new_neighbors_flux_window, current_time, 1.0);
}

bool DE_computeNewNeighborsFlux(DiscoveryEnvironment* de, struct timespec* current_time, char* window_type, double epsilon, int precision) {
    assert(de);
    double new_new_neighbors_flux = computeWindow(de->new_neighbors_flux_window, current_time, window_type, "sum", true);
    new_new_neighbors_flux = roundPrecision(new_new_neighbors_flux, precision);

    double delta = fabs(new_new_neighbors_flux - de->old_new_neighbors_flux);
    if( delta >= epsilon || (delta > 0 && (new_new_neighbors_flux == 0.0)) ) {
        de->old_new_neighbors_flux = new_new_neighbors_flux;
        return true;
    }
    return false;
}

void DE_registerLostNeighbor(DiscoveryEnvironment* de, struct timespec* current_time) {
    assert(de);
    insertIntoWindow(de->lost_neighbors_flux_window, current_time, 1.0);
}

bool DE_computeLostNeighborsFlux(DiscoveryEnvironment* de, struct timespec* current_time, char* window_type, double epsilon, int precision) {
    assert(de);
    double new_lost_neighbors_flux = computeWindow(de->lost_neighbors_flux_window, current_time, window_type, "sum", true);
    new_lost_neighbors_flux = roundPrecision(new_lost_neighbors_flux, precision);

    double delta = fabs(new_lost_neighbors_flux - de->old_lost_neighbors_flux);
    if( delta >= epsilon || (delta > 0 && (new_lost_neighbors_flux == 0.0)) ) {
        de->old_lost_neighbors_flux = new_lost_neighbors_flux;
        return true;
    }
    return false;
}

double DE_getInTraffic(DiscoveryEnvironment* de) {
    assert(de);
    return de->old_in_traffic;
}

double DE_getOutTraffic(DiscoveryEnvironment* de) {
    assert(de);
    return de->old_out_traffic;
}

double DE_getNewNeighborsFlux(DiscoveryEnvironment* de) {
    assert(de);
    return de->old_new_neighbors_flux;
}

double DE_getLostNeighborsFlux(DiscoveryEnvironment* de) {
    assert(de);
    return de->old_lost_neighbors_flux;
}

bool DE_setInTraffic(DiscoveryEnvironment* de, double new_in_traffic, double epsilon) {
    assert(de);
    double delta = fabs(new_in_traffic - de->old_in_traffic);
    if( delta >= epsilon || (delta > 0 && (new_in_traffic == 0.0)) ) {
        de->old_in_traffic = new_in_traffic;
        return true;
    }
    return false;
}

unsigned int DE_getNNeighbors(DiscoveryEnvironment* de) {
    assert(de);
    return de->old_n_neighbors;
}

bool DE_setNNeighbors(DiscoveryEnvironment* de, unsigned int new_n_neighbors) {
    assert(de);

    if( new_n_neighbors != de->old_n_neighbors ) {
        de->old_n_neighbors = new_n_neighbors;
        return true;
    }
    return false;
}

bool DE_setNeigbhorsDensity(DiscoveryEnvironment* de, double new_neighbors_density, double epsilon) {
    assert(de);
    double delta = fabs(new_neighbors_density - de->old_neighbors_density);
    if( delta >= epsilon || (delta > 0 && (new_neighbors_density == 0.0)) ) {
        de->old_neighbors_density = new_neighbors_density;
        return true;
    }
    return false;
}

double DE_getNeigbhorsDensity(DiscoveryEnvironment* de) {
    assert(de);
    return de->old_neighbors_density;
}

char* NE_print(DiscoveryEnvironment* de, char** str) {

    char* buffer = malloc(300*sizeof(char));

    char* headers = "  OUT TRAFFIC  |  IN TRAFFIC  | NEW NEIGHBORS | LOST NEIGHBORS | N NEIGHBORS | NEIGH DENSITY";

    sprintf(buffer, "%s\n  %0.3f msgs/s   %0.3f msgs/s       %0.3f            %0.3f             %u           %0.3f",
        headers,
        DE_getOutTraffic(de),
        DE_getInTraffic(de),
        DE_getNewNeighborsFlux(de),
        DE_getLostNeighborsFlux(de),
        DE_getNNeighbors(de),
        DE_getNeigbhorsDensity(de)
    );

    *str = buffer;
    return buffer;
}
