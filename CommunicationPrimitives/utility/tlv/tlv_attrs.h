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

#ifndef _UTILITY_TLV_ATTRS_H_
#define _UTILITY_TLV_ATTRS_H_

#include "tlv_private.h"

typedef unsigned int (*TLVT_parse_)(TLVTuple* tlv, byte* buffer);

typedef TLVTuple* (*TLVT_unparse_)(byte* buffer, unsigned int length);

typedef TLVTuple* (*TLVT_destroy_)(void* value);

typedef struct TLVTypeAtrrs_ {
    int length_size;
    TLVT_parse_ parse;
    TLVT_unparse_ unparse;
    TLVT_destroy_ destroy;
} TLVTypeAttrs;

extern TLVTypeAttrs tlv_type[];

#endif /*_UTILITY_TLV_ATTRS_H_*/
