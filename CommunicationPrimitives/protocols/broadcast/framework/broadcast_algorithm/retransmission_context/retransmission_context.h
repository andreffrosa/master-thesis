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

#include "data_structures/list.h"
#include "data_structures/hash_table.h"

#include "../common.h"

typedef struct _RetransmissionContext RetransmissionContext;

void RC_init(RetransmissionContext* context, proto_def* protocol_definition, unsigned char* myID, list* visited);

void RC_processEvent(RetransmissionContext* context, queue_t_elem* elem, unsigned char* myID, list* visited);

unsigned int RC_createHeader(RetransmissionContext* context, PendingMessage* p_msg, void** context_header, unsigned char* myID, list* visited);

bool RC_query(RetransmissionContext* context, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited);

bool RC_queryHeader(RetransmissionContext* context, void* context_header, unsigned int context_header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited);

void RC_processCopy(RetransmissionContext* context, PendingMessage* p_msg, unsigned char* myID, list* visited);

void destroyRetransmissionContext(RetransmissionContext* context, list* visited);

/*
bool query_context(RetransmissionContext* root_context, char* query, void* result, unsigned char* myID, int argc, ...);

bool query_context_header(RetransmissionContext* root_context, void* header, unsigned int length, char* query, void* result, unsigned char* myID, int argc, ...);
*/

//////////////////////////////////////////////////////////////////////////////////////////

RetransmissionContext* EmptyContext();

RetransmissionContext* HopsContext();

RetransmissionContext* ParentsContext(unsigned int max_amount);

RetransmissionContext* RouteContext(unsigned int max_len);

RetransmissionContext* NeighborsContext();

RetransmissionContext* LENWBContext(RetransmissionContext* neighbors_context);

RetransmissionContext* LabelNeighsContext(RetransmissionContext* neighbors_context);
typedef enum NeighCoverageLabel_ {
	UNKNOWN_NODE, CRITICAL_NODE, COVERED_NODE, REDUNDANT_NODE
} NeighCoverageLabel;

RetransmissionContext* ComposeContext(int amount, ...);

RetransmissionContext* MultiPointRelayContext(RetransmissionContext* neighbors_context);

RetransmissionContext* AHBPContext(RetransmissionContext* neighbors_context, RetransmissionContext* route_context);

RetransmissionContext* DynamicProbabilityContext(double p, double p_l, double p_u, double d, unsigned long t);

RetransmissionContext* HopCountAwareRADExtensionContext(unsigned long delta_t);

RetransmissionContext* LatencyContext();


#endif /* _RETRANSMISSION_CONTEXT_H_ */
