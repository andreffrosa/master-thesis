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

#ifndef _ROUTING_ALGORITHM_H_
#define _ROUTING_ALGORITHM_H_

#include <stdarg.h>

#include "common.h"

#include "routing_context/routing_context.h"
#include "forwarding_strategy/forwarding_strategy.h"
#include "cost_metric/cost_metric.h"
#include "announce_period/announce_period.h"
#include "dissemination_strategy/dissemination_strategy.h"

typedef struct RoutingAlgorithm_ RoutingAlgorithm;

RoutingAlgorithm* newRoutingAlgorithm(RoutingContext* r_context, ForwardingStrategy* f_strategy, CostMetric* cost_metric, AnnouncePeriod* a_period, DisseminationStrategy* d_strategy);

void destroyRoutingAlgorithm(RoutingAlgorithm* algorithm);

void RA_setRoutingContext(RoutingAlgorithm* alg, RoutingContext* new_r_context);

void RA_setForwardingStrategy(RoutingAlgorithm* alg, ForwardingStrategy* new_f_strategy);

void RA_setCostMetric(RoutingAlgorithm* alg, CostMetric* new_cost_metric);

void RA_setAnnouncePeriod(RoutingAlgorithm* alg, AnnouncePeriod* new_a_period);

void RA_setDisseminationStrategy(RoutingAlgorithm* alg, DisseminationStrategy* new_d_strategy);

bool RA_getNextHop(RoutingAlgorithm* alg, RoutingTable* routing_table, SourceTable* source_table, RoutingNeighbors* neighbors, unsigned char* myID, unsigned char* destination_id, unsigned char* next_hop_id, WLANAddr* next_hop_addr, byte** meta_data, unsigned short* meta_data_length, byte* prev_meta_data, unsigned short prev_meta_data_length, bool first, struct timespec* current_time);

void RA_init(RoutingAlgorithm* alg, proto_def* protocol_definition, unsigned char* myID, RoutingTable* r_table, struct timespec* current_time);

void RA_computeCost(RoutingAlgorithm* alg, bool is_bi, double rx_lq, double tx_lq, struct timespec* found_time, double* rx_cost, double* tx_cost);

unsigned int RA_getAnnouncePeriod(RoutingAlgorithm* alg);

void RA_disseminateControlMessage(RoutingAlgorithm* alg, unsigned char* myID, YggMessage* msg, RoutingEventType event_type, void* info);

RoutingContextSendType RA_triggerEvent(RoutingAlgorithm* alg, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time);

void RA_createControlMsg(RoutingAlgorithm* alg, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info);

RoutingContextSendType RA_processControlMsg(RoutingAlgorithm* alg, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, unsigned short src_proto, byte* meta_data, unsigned int meta_length);

#endif /* _ROUTING_ALGORITHM_H_ */
