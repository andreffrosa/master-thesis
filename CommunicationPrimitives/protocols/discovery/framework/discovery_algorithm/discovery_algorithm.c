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

DiscoveryAlgorithm* newDiscoveryAlgorithm(DiscoveryPattern* d_pattern, DiscoveryPeriod* d_period, LinkQuality* lq_metric, LinkAdmission* la_policy, DiscoveryContext* d_context) {
    assert(d_pattern && d_period && lq_metric && la_policy && d_context);

    DiscoveryAlgorithm* alg = (DiscoveryAlgorithm*)malloc(sizeof(DiscoveryAlgorithm));

    alg->d_pattern = d_pattern;
    alg->d_period = d_period;
    alg->lq_metric = lq_metric;
    alg->la_policy = la_policy;
    alg->d_context = d_context;

	return alg;
}

void destroyDiscoveryAlgorithm(DiscoveryAlgorithm* alg) {
    if(alg != NULL) {
        destroyDiscoveryPattern(alg->d_pattern);
        destroyDiscoveryPeriod(alg->d_period);
        destroyLinkQualityMetric(alg->lq_metric);
        destroyLinkAdmissionPolicy(alg->la_policy);
        destroyDiscoveryContext(alg->d_context);
        free(alg);
    }
}

void DA_setDiscoveryPattern(DiscoveryAlgorithm* alg, DiscoveryPattern* new_d_pattern) {
    assert(alg && new_d_pattern);

    if( alg->d_pattern ) {
        destroyDiscoveryPattern(alg->d_pattern);
    }

    alg->d_pattern = new_d_pattern;
}

void DA_setDiscoveryPeriod(DiscoveryAlgorithm* alg, DiscoveryPeriod* new_d_period) {
    assert(alg && new_d_period);

    if( alg->d_period ) {
        destroyDiscoveryPeriod(alg->d_period);
    }

    alg->d_period = new_d_period;
}

void DA_setLinkQuality(DiscoveryAlgorithm* alg, LinkQuality* new_lq_metric) {
    assert(alg && new_lq_metric);

    if( alg->lq_metric ) {
        destroyLinkQualityMetric(alg->lq_metric);
    }

    alg->lq_metric = new_lq_metric;
}

void DA_setLinkAdmission(DiscoveryAlgorithm* alg, LinkAdmission* new_la_policy) {
    assert(alg && new_la_policy);

    if( alg->la_policy ) {
        destroyLinkAdmissionPolicy(alg->la_policy);
    }

    alg->la_policy = new_la_policy;
}

void DA_setDiscoveryContext(DiscoveryAlgorithm* alg, DiscoveryContext* new_d_context) {
    assert(alg && new_d_context);

    if(alg->d_context) {
        destroyDiscoveryContext(alg->d_context);
    }

    alg->d_context = new_d_context;
}

PeriodicType DA_periodicHello(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_periodicType(DP_getHelloScheduler(alg->d_pattern));
}

/*
PiggybackType DA_piggybackHellos(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_piggybackType(DP_getHelloScheduler(alg->d_pattern));
}
*/

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

bool DA_HelloContextUpdate(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_updateContext(DP_getHelloScheduler(alg->d_pattern));
}

HelloSchedulerType DA_getHelloType(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HELLO_getType(DP_getHelloScheduler(alg->d_pattern));
}

PeriodicType DA_periodicHack(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_periodicType(DP_getHackScheduler(alg->d_pattern));
}

/*
PiggybackType DA_piggybackHacks(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_piggybackType(DP_getHackScheduler(alg->d_pattern));
}
*/

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

bool DA_HackContextUpdate(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_updateContext(DP_getHackScheduler(alg->d_pattern));
}

HelloSchedulerType DA_getHackType(DiscoveryAlgorithm* alg) {
    assert(alg);
    return HACK_getType(DP_getHackScheduler(alg->d_pattern));
}

DiscoveryInternalEventResult* DA_triggerEvent(DiscoveryAlgorithm* alg, DiscoveryInternalEventType event_type, void* event_args, NeighborsTable* neighbors, YggMessage* msg) {
    assert(alg);
    return DP_triggerEvent(alg->d_pattern, event_type, event_args, neighbors, msg);
}

/*
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
*/

byte DA_getHelloAnnouncePeriod(DiscoveryAlgorithm* alg) {
    assert(alg);
    return DP_getHelloAnnouncePeriod(alg->d_period);
}

byte DA_getHelloTransmitPeriod(DiscoveryAlgorithm* alg, struct timespec* current_time) {
    assert(alg);
    return DP_getHelloTransmitPeriod(alg->d_period, current_time);
}

byte DA_getHackAnnouncePeriod(DiscoveryAlgorithm* alg) {
    assert(alg);
    return DP_getHackAnnouncePeriod(alg->d_period);
}

byte DA_getHackTransmitPeriod(DiscoveryAlgorithm* alg, struct timespec* current_time) {
    assert(alg);
    return DP_getHackTransmitPeriod(alg->d_period, current_time);
}

byte DA_computeNextHelloPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, unsigned int transition_period_n, NeighborsTable* neighbors, struct timespec* current_time) {
    assert(alg);
    return DP_computeNextHackPeriod(alg->d_period, elapsed_time_ms, transition_period_n, neighbors, current_time);
}

byte DA_computeNextHackPeriod(DiscoveryAlgorithm* alg, unsigned long elapsed_time_ms, unsigned int transition_period_n, NeighborsTable* neighbors, struct timespec* current_time) {
    assert(alg);
    return DP_computeNextHackPeriod(alg->d_period, elapsed_time_ms, transition_period_n, neighbors, current_time);
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

bool DA_evalLinkAdmission(DiscoveryAlgorithm* alg, NeighborEntry* neigh, struct timespec* current_time) {
    assert(alg != NULL);

    return LA_eval(alg->la_policy, neigh, current_time);
}

void DA_createMessage(DiscoveryAlgorithm* alg, unsigned char* myID, NeighborsTable* neighbors, DiscoveryInternalEventType event_type, void* event_args, struct timespec* current_time, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size) {
    assert(alg != NULL);

    DC_create(alg->d_context, myID, neighbors, event_type, event_args, current_time, hello, hacks, n_hacks, buffer, size);
}

bool DA_processMessage(DiscoveryAlgorithm* alg, void* f_state, unsigned char* myID, NeighborsTable* neighbors, struct timespec* current_time, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size, MessageSummary* msg_summary) {
    assert(alg != NULL);

    return DC_process(alg->d_context, f_state, myID, neighbors, current_time, piggybacked, mac_addr, buffer, size, msg_summary);
}

bool DA_updateContext(DiscoveryAlgorithm* alg, unsigned char* myID, NeighborEntry* neighbor, NeighborsTable* neighbors, struct timespec* current_time, NeighborTimerSummary* summary) {
    assert(alg != NULL);

    return DC_update(alg->d_context, myID, neighbor, neighbors, current_time, summary);
}

void* DA_createContextAttributes(DiscoveryAlgorithm* alg) {
    assert(alg != NULL);

    return DC_createAttrs(alg->d_context);
}

void DA_destroyContextAttributes(DiscoveryAlgorithm* alg, void* context_attributes) {
    assert(alg != NULL);

    DC_destroyAttrs(alg->d_context, context_attributes);
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
