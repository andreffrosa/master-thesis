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

void RC_init(RetransmissionContext* context, proto_def* protocol_definition, unsigned char* myID);

void RC_processEvent(RetransmissionContext* context, queue_t_elem* elem, unsigned char* myID, hash_table* contexts);

//unsigned short RC_createHeader(RetransmissionContext* context, PendingMessage* p_msg, byte** context_header, unsigned char* myID);

void RC_appendHeaders(RetransmissionContext* context, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time);

void RC_parseHeaders(RetransmissionContext* context, hash_table* serialized_headers, hash_table* headers, unsigned char* myID);

// bool RC_queryHeader(RetransmissionContext* context, void* context_header, unsigned int context_header_size, const char* query, void* result, hash_table* query_args, unsigned char* myID);

bool RC_query(RetransmissionContext* context, const char* query, void* result, hash_table* query_args, unsigned char* myID, hash_table* contexts);

void RC_processCopy(RetransmissionContext* context, PendingMessage* p_msg, unsigned char* myID);

const char* RC_getID(RetransmissionContext* context);

list* RC_getDependencies(RetransmissionContext* context);

//void RC_addDependency(RetransmissionContext* context, char* dependency);

void destroyRetransmissionContext(RetransmissionContext* context);

/*
bool query_context(RetransmissionContext* root_context, char* query, void* result, unsigned char* myID, int argc, ...);

bool query_context_header(RetransmissionContext* root_context, void* header, unsigned int length, char* query, void* result, unsigned char* myID, int argc, ...);
*/

//////////////////////////////////////////////////////////////////////////////////////////

RetransmissionContext* EmptyContext();

RetransmissionContext* HopsContext(char* type);

RetransmissionContext* ParentsContext(unsigned int max_amount);

RetransmissionContext* RouteContext(unsigned int max_len);

RetransmissionContext* NeighborsContext();

RetransmissionContext* LENWBContext();

RetransmissionContext* LabelNeighsContext();
typedef enum NeighCoverageLabel_ {
    UNKNOWN_NODE, CRITICAL_NODE, COVERED_NODE, REDUNDANT_NODE
} NeighCoverageLabel;

RetransmissionContext* MultiPointRelayContext();

RetransmissionContext* AHBPContext();

RetransmissionContext* DynamicProbabilityContext(double p, double p_l, double p_u, double d, unsigned long t);

RetransmissionContext* HopCountAwareRADExtensionContext(unsigned long delta_t);

RetransmissionContext* LatencyContext();

RetransmissionContext* BiFloodingContext();

RetransmissionContext* BATMANContext();


#endif /* _RETRANSMISSION_CONTEXT_H_ */
