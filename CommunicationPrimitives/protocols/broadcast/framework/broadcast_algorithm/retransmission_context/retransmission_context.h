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

#ifndef _RETRANSMISSION_CONTEXT_H_
#define _RETRANSMISSION_CONTEXT_H_

#include "protocols/discovery/topology_discovery.h"

#include "data_structures/graph.h"

typedef struct _RetransmissionContext RetransmissionContext;

void destroyRetransmissionContext(RetransmissionContext* context, list* visited);

bool query_context(RetransmissionContext* r_context, char* query, void* result, unsigned char* myID, int argc, ...);

bool query_context_header(RetransmissionContext* r_context, void* header, unsigned int length, char* query, void* result, unsigned char* myID, int argc, ...);

//////////////////////////////////////////////////////////////////////////////////////////

RetransmissionContext* EmptyContext();

RetransmissionContext* HopsContext();

RetransmissionContext* ParentsContext(unsigned int max_amount) ;

RetransmissionContext* RouteContext(unsigned int max_len);

RetransmissionContext* NeighborsContext(unsigned int window_size, topology_discovery_args* d_args);

RetransmissionContext* LabelNeighsContext(RetransmissionContext* neighbors_context);
typedef enum _LabelNeighs_NodeLabel {
	UNKNOWN_NODE, CRITICAL_NODE, COVERED_NODE, REDUNDANT_NODE
} LabelNeighs_NodeLabel;

RetransmissionContext* ComposeContext(int amount, ...);

RetransmissionContext* MultiPointRelayContext(RetransmissionContext* neighbors_context, double hyst_threshold_low, double hyst_threshold_high);

RetransmissionContext* AHBPContext(RetransmissionContext* neighbors_context, RetransmissionContext* route_context, double hyst_threshold_low, double hyst_threshold_high);

RetransmissionContext* DynamicProbabilityContext(double p, double p_l, double p_u, double d, unsigned long t);

RetransmissionContext* HopCountAwareRADExtensionContext(unsigned long delta_t);

RetransmissionContext* MonitorContext(unsigned long log_period, char* window_type, RetransmissionContext* child_context);

RetransmissionContext* LatencyContext();

#endif /* _RETRANSMISSION_CONTEXT_H_ */
