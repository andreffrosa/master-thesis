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

#ifndef _ROUTING_FRAMEWORK_FORWARDING_PLANE_H_
#define _ROUTING_FRAMEWORK_FORWARDING_PLANE_H_

#include "handlers.h"

void RF_DeliverMessage(routing_framework_state* state, RoutingHeader* header, YggMessage* toDeliver);

void RF_uponRouteRequest(routing_framework_state* state, YggRequest* req);

void RF_uponNewMessage(routing_framework_state* state, YggMessage* msg);

void RF_processMessage(routing_framework_state* state, RoutingHeader* header, byte* prev_meta_data, bool first, YggMessage* toDeliver);
void RF_ForwardMessage(routing_framework_state* state, RoutingHeader* header, byte* prev_meta_data, unsigned short ttl, bool first, YggMessage* toDeliver);

void RF_SendMessage(routing_framework_state* state, RoutingHeader* old_header, unsigned char* next_hop_id, unsigned char* next_hop_addr, byte* new_meta_data, unsigned short new_meta_data_length, unsigned short ttl, YggMessage* toDeliver);

void RF_serializeMessage(routing_framework_state* state, YggMessage* m, RoutingHeader* old_header, unsigned char* next_hop_id, unsigned char* next_hop_addr, byte* new_meta_data, unsigned short new_meta_data_length, unsigned short ttl, YggMessage* toDeliver);

void RF_deserializeMessage(YggMessage* m, RoutingHeader* header, byte* meta_data, YggMessage* toDeliver);

bool RF_findNextHop(routing_framework_state* state, unsigned char* destination_id, unsigned char* next_hop_id, unsigned char* next_hop_addr);


#endif /* _ROUTING_FRAMEWORK_FORWARDING_PLANE_H_ */
