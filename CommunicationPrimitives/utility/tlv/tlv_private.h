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

#ifndef _UTILITY_TLV_PRIVATE_H_
#define _UTILITY_TLV_PRIVATE_H_

#include "tlv.h"

#include "data_structures/hash_table.h"

typedef struct TLVMessage_ {
    hash_table* ht;
} TLVMessage;

typedef struct TLVTuple_ {
    TLVType type;
    void* value;
} TLVTuple;

#endif /*_UTILITY_TLV_PRIVATE_H_*/
