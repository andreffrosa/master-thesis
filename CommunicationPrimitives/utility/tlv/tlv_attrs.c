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

#include "tlv_attrs.h"

#include "utility/my_misc.h"

#include <uuid/uuid.h>

#include <assert.h>



static unsigned int TLV_ID_parse(TLVTuple* tlv, byte* buffer) {
    assert(tlv && buffer);

    uuid_copy(buffer, tlv->value);
    return sizeof(uuid_t);
}

static TLVTuple* TLV_ID_unparse(byte* buffer, unsigned int length) {
    assert(buffer);
    assert(length == sizeof(uuid_t));

    return newTLVTuple(TLV_SENDER_ID, new_id(buffer));
}

TLVTypeAttrs tlv_type[TLV_COUNT] = {
    {.length_size = 1, .parse  = &TLV_ID_parse, .unparse = &TLV_ID_unparse, .destroy = NULL} //TLV_SENDER_ID
};
