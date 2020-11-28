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

#ifndef _BCAST_HEADER_H_
#define _BCAST_HEADER_H_

#include "Yggdrasil.h"

#pragma pack(1)
typedef struct bcast_header_ {
	uuid_t source_id;
    uuid_t sender_id;
	uuid_t msg_id;
	unsigned short dest_proto;
    unsigned short ttl;
	unsigned short context_length;
} bcast_header;
#pragma pack()

#define BCAST_HEADER_LENGTH sizeof(bcast_header)

#endif /* _BCAST_HEADER_H_ */
