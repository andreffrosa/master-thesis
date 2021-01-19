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

#include "routing_algorithm_private.h"

RoutingAlgorithm* newRoutingAlgorithm(RoutingContext* r_context, ForwardingStrategy* f_strategy, CostMetric* cost_metric, AnnouncePeriod* a_period, DisseminationStrategy* d_strategy) {
    assert(r_context && f_strategy && cost_metric && a_period && d_strategy);

	RoutingAlgorithm* alg = (RoutingAlgorithm*)malloc(sizeof(RoutingAlgorithm));

    alg->r_context = r_context;
    alg->f_strategy = f_strategy;
    alg->cost_metric = cost_metric;
    alg->a_period = a_period;
    alg->d_strategy = d_strategy;

	return alg;
}

void destroyRoutingAlgorithm(RoutingAlgorithm* alg) {
    if(alg != NULL) {
        destroyRoutingContext(alg->r_context);
        destroyForwardingStrategy(alg->f_strategy);
        destroyCostMetric(alg->cost_metric);
        destroyAnnouncePeriod(alg->a_period);
        destroyDisseminationStrategy(alg->d_strategy);

        free(alg);
    }
}

void RA_setRoutingContext(RoutingAlgorithm* alg, RoutingContext* new_r_context) {
    assert(alg);

    destroyRoutingContext(alg->r_context);
    alg->r_context = new_r_context;
}

void RA_setForwardingStrategy(RoutingAlgorithm* alg, ForwardingStrategy* new_f_strategy) {
    assert(alg);

    destroyForwardingStrategy(alg->f_strategy);
    alg->f_strategy = new_f_strategy;
}

void RA_setCostMetric(RoutingAlgorithm* alg, CostMetric* new_cost_metric) {
    assert(alg);

    destroyCostMetric(alg->cost_metric);
    alg->cost_metric = new_cost_metric;
}

void RA_setAnnouncePeriod(RoutingAlgorithm* alg, AnnouncePeriod* new_a_period) {
    assert(alg);

    destroyAnnouncePeriod(alg->a_period);
    alg->a_period = new_a_period;
}

void RA_setDisseminationStrategy(RoutingAlgorithm* alg, DisseminationStrategy* new_d_strategy) {
    assert(alg);

    destroyDisseminationStrategy(alg->d_strategy);
    alg->d_strategy = new_d_strategy;
}

bool RA_getNextHop(RoutingAlgorithm* alg, RoutingTable* routing_table, unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, struct timespec* current_time) {
    assert(alg);

    return FS_getNextHop(alg->f_strategy, routing_table, destination_id, next_hop_id, next_hop_addr, current_time);
}

void RA_init(RoutingAlgorithm* alg, proto_def* protocol_definition, unsigned char* myID, RoutingTable* r_table, struct timespec* current_time) {
    assert(alg);

    RCtx_init(alg->r_context, protocol_definition, myID, r_table, current_time);
}

void RA_computeCost(RoutingAlgorithm* alg, bool is_bi, double rx_lq, double tx_lq, struct timespec* found_time, double* rx_cost, double* tx_cost) {
    assert(alg);

    CM_compute(alg->cost_metric, is_bi, rx_lq, tx_lq, found_time, rx_cost, tx_cost);
}

unsigned int RA_getAnnouncePeriod(RoutingAlgorithm* alg) {
    assert(alg);

    return AP_get(alg->a_period);
}

void RA_disseminateControlMessage(RoutingAlgorithm* alg, unsigned char* myID, YggMessage* msg, RoutingEventType event_type, void* info) {
    assert(alg);

    DS_disseminate(alg->d_strategy, myID, msg, event_type, info);
}

RoutingContextSendType RA_triggerEvent(RoutingAlgorithm* alg, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {
    assert(alg);

    return RCtx_triggerEvent(alg->r_context, event_type, args, routing_table, neighbors, source_table, myID, current_time);
}

void RA_createControlMsg(RoutingAlgorithm* alg, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info) {
    assert(alg);

    RCtx_createMsg(alg->r_context, header, routing_table, neighbors, source_table, myID, current_time, msg, event_type, info);
}

RoutingContextSendType RA_processControlMsg(RoutingAlgorithm* alg, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, unsigned short src_proto, byte* meta_data, unsigned int meta_length, bool* forward) {
    assert(alg);

    return RCtx_processMsg(alg->r_context, routing_table, neighbors, source_table, source_entry, myID, current_time, header, payload, length, src_proto, meta_data, meta_length, forward);
}
