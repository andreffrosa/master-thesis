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

#include <stdlib.h>
#include <assert.h>

#include "discovery_algorithm_private.h"

#include "utility/my_misc.h"

DiscoveryAlgorithm* newDiscoveryAlgorithm(DiscoveryPattern* d_pattern, DiscoveryPeriod* d_period, LinkQuality* lq_metric, DiscoveryMessage* d_message) {
    assert(d_pattern && d_period && lq_metric && d_message);

    DiscoveryAlgorithm* alg = (DiscoveryAlgorithm*)malloc(sizeof(DiscoveryAlgorithm));

    alg->d_pattern = d_pattern;
    alg->d_period = d_period;
    alg->lq_metric = lq_metric;
    alg->d_message = d_message;

	return alg;
}

void destroyDiscoveryAlgorithm(DiscoveryAlgorithm* alg) {
    if(alg != NULL) {
        destroyDiscoveryPattern(alg->d_pattern);
        destroyDiscoveryPeriod(alg->d_period);
        destroyLinkQualityMetric(alg->lq_metric);
        destroyDiscoveryMessage(alg->d_message);
        free(alg);
    }
}



bool DA_periodicHello(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_isPeriodic(DP_getHelloScheduler(alg->d_pattern));
}

PiggybackType DA_piggybackHellos(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_piggybackType(DP_getHelloScheduler(alg->d_pattern));
}

bool DA_HelloNewNeighbor(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_newNeighbor(DP_getHelloScheduler(alg->d_pattern));
}

bool DA_HelloLostNeighbor(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_lostNeighbor(DP_getHelloScheduler(alg->d_pattern));
}

bool DA_HelloUpdateNeighbor(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_updateNeighbor(DP_getHelloScheduler(alg->d_pattern));
}

HelloSchedulerType DA_getHelloType(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_getType(DP_getHelloScheduler(alg->d_pattern));
}

bool DA_periodicHack(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_isPeriodic(DP_getHackScheduler(alg->d_pattern));
}

PiggybackType DA_piggybackHacks(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_piggybackType(DP_getHackScheduler(alg->d_pattern));
}

HackReplyType DA_replyHacksToHellos(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_replyToHellos(DP_getHackScheduler(alg->d_pattern));
}

bool DA_HackNewNeighbor(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_newNeighbor(DP_getHackScheduler(alg->d_pattern));
}

bool DA_HackLostNeighbor(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_lostNeighbor(DP_getHackScheduler(alg->d_pattern));
}

bool DA_HackUpdateNeighbor(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_updateNeighbor(DP_getHackScheduler(alg->d_pattern));
}

HelloSchedulerType DA_getHackType(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_getType(DP_getHackScheduler(alg->d_pattern));
}

byte DA_getHelloPeriod(DiscoveryAlgorithm* alg) {
    assert(alg);
    return DP_getHelloPeriod(alg->d_period);
}

byte DA_getHackPeriod(DiscoveryAlgorithm* alg) {
    assert(alg);
    return DP_getHackPeriod(alg->d_period);
}

byte DA_computeNextHelloPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, NeighborsTable* neighbors) {
    assert(alg);
    return DP_computeNextHelloPeriod(alg->d_period, elapsed_time_ms, neighbors);
}

byte DA_computeNextHackPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, NeighborsTable* neighbors) {
    assert(alg);
    return DP_computeNextHackPeriod(alg->d_period, elapsed_time_ms, neighbors);
}

double DA_computeLinkQuality(DiscoveryAlgorithm* alg, void* lq_attrs, double previous_link_quality, unsigned int received, unsigned int lost, bool init, struct timespec* current_time) {
    assert(alg != NULL);

    return LQ_compute(alg->lq_metric, lq_attrs, previous_link_quality, received, lost, init, current_time);
}

void* DA_createLinkQualityAttributes(DiscoveryAlgorithm* alg) {
    assert(alg != NULL);

    return LQ_createAttrs(alg->lq_metric);
}

void DA_destroyLinkQualityAttributes(DiscoveryAlgorithm* alg, void* lq_attrs) {
    assert(alg != NULL);

    LQ_destroyAttrs(alg->lq_metric, lq_attrs);
}

void DA_createDiscoveryMessage(DiscoveryAlgorithm* alg, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size) {
    assert(alg != NULL);

    DM_create(alg->d_message, myID, current_time, neighbors, piggybacked, hello, hacks, n_hacks, buffer, size);
}

bool DA_processDiscoveryMessage(DiscoveryAlgorithm* alg, void* f_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size) {
    assert(alg != NULL);

    return DM_process(alg->d_message, f_state, myID, current_time, neighbors, piggybacked, mac_addr, buffer, size);
}

void* DA_createMessageAttributes(DiscoveryAlgorithm* alg) {
    assert(alg != NULL);

    return DM_createAttrs(alg->d_message);
}

void DA_destroyMessageAttributes(DiscoveryAlgorithm* alg, void* msg_attributes) {
    assert(alg != NULL);

    DM_destroyAttrs(alg->d_message, msg_attributes);
}






/*
unsigned long computeHeartbeatTimer(DiscoveryAlgorithm* alg) {
    assert(alg != NULL);
    // TODO
    return alg->hb_timer->hb_timer(&alg->hb_timer->state);
}

DiscoveryType getDiscoveryType(DiscoveryAlgorithm* alg) {
    assert(alg != NULL);
    return alg->hb_type;
}

*/

/*
bool createNewDiscoveryAnnounce(DiscoveryAlgorithm* alg, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, list* triggered_events, unsigned char** announce, unsigned int* announce_size) {
    assert(alg != NULL);
    if(alg->announce_module)
        return alg->announce_module->create_announce(&alg->announce_module->state, myID, current_time, neighbors, triggered_events, announce, announce_size);
    else
        return false;
}

bool processDiscoveryAnnounce(DiscoveryAlgorithm* alg, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, NeighborEntry* neigh, bool new_neighbor, unsigned char* announce, unsigned int announce_size, bool* is_bi, bool* updated_attrs) {
    assert(alg != NULL);
    if(alg->announce_module)
        return alg->announce_module->process_announce(&alg->announce_module->state, myID, current_time, neighbors, neigh, new_neighbor, announce, announce_size, is_bi, updated_attrs);
    else
        return false;
}
*/



/*
hash_table* getSubscribedEvents(DiscoveryAlgorithm* alg) {
    assert(alg != NULL);
    if(alg->announce_module)
        return alg->announce_module->subscriptions;
    else
        return NULL;
}

bool imSubscribedTo(DiscoveryAlgorithm* alg, unsigned int proto_id, int event_id) {
    assert(alg != NULL);

    hash_table* subscriptions = getSubscribedEvents(alg);
    if(subscriptions != NULL)
        return check_subscription(subscriptions, proto_id, event_id);
    else
        return false;
}


*/