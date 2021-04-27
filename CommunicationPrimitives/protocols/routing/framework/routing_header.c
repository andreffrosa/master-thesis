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

#include "routing_header.h"

#include <assert.h>

void initRoutingHeader(RoutingHeader* header, unsigned char* source_id, unsigned char* prev_hop_id, unsigned char* next_hop_id, unsigned char* destination_id, unsigned char* msg_id, unsigned short ttl, unsigned short dest_proto, bool hop_delivery, unsigned short meta_data_length) {
    assert(header);

    uuid_copy(header->source_id, source_id);
    uuid_copy(header->prev_hop_id, prev_hop_id);
    uuid_copy(header->next_hop_id, next_hop_id);
    uuid_copy(header->destination_id, destination_id);
    uuid_copy(header->msg_id, msg_id);
    header->ttl = ttl;
    header->dest_proto = dest_proto;
    header->hop_delivery = hop_delivery;
    header->meta_data_length = meta_data_length;
}

void initRoutingControlHeader(RoutingControlHeader* header, /*unsigned char* source_id,*/ unsigned short seq, byte announce_period) {
    assert(header);

    //uuid_copy(header->source_id, source_id);
    header->seq = seq;
    header->announce_period = announce_period;
}
