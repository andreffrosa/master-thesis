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
 * (C) 2019
 *********************************************************/

#ifndef _ROUTING_HEADER_H_
#define _ROUTING_HEADER_H_

#include "Yggdrasil.h"

#include "utility/byte.h"

#pragma pack(1)
typedef struct RoutingHeader_ {
	uuid_t source_id;
    uuid_t prev_hop_id;
    uuid_t next_hop_id;
    uuid_t destination_id;
	uuid_t msg_id;
    unsigned short ttl;
	unsigned short dest_proto;
    byte hop_delivery;
	//unsigned short payload_length;
} RoutingHeader;
#pragma pack()

#pragma pack(1)
typedef struct RoutingControlHeader_ {
	//uuid_t source_id;
	unsigned short seq;
    byte announce_period;
    //unsigned short ttl; // (?)
} RoutingControlHeader;
#pragma pack()

// #define ROUTING_HEADER_LENGTH sizeof(RoutingHeader)

void initRoutingHeader(RoutingHeader* header, unsigned char* source_id, unsigned char* prev_hop_id, unsigned char* next_hop_id, unsigned char* destination_id, unsigned char* msg_id, unsigned short ttl, unsigned short dest_proto, bool hop_delivery);

void initRoutingControlHeader(RoutingControlHeader* header, /*unsigned char* source_id,*/ unsigned short seq, byte announce_period);

#endif /* _ROUTING_HEADER_H_ */
