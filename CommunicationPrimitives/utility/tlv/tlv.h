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

#ifndef _UTILITY_TLV_H_
#define _UTILITY_TLV_H_

#include "byte.h"

#define TLV_MESSAGE_MAX_SIZE 1500

typedef enum {
    TLV_SENDER_ID,
    TLV_APP_PAYLOAD,
    TLV_BROADCAST_MSG_ID,
    TLV_COUNT
} TLVType;

typedef struct TLVMessage_ TLVMessage;
typedef struct TLVTuple_ TLVTuple;

TLVMessage* newTLVMessage();

void destroyTLVMessage(TLVMessage* tlv_msg);

void TLVM_add(TLVMessage* tlv_msg, TLVTuple* tlv_tuple);

TLVTuple* TLVM_get(TLVMessage* tlv_msg, TLVType type);

TLVTuple* TLVM_remove(TLVMessage* tlv_msg, TLVType type);

TLVTuple* TLVM_next(TLVMessage* tlv_msg, void** iterator);

unsigned short TLVM_parse(TLVMessage* tlv_msg, byte** buffer);

TLVMessage* TLVM_unparse(byte* buffer, unsigned short size);

TLVTuple* newTLVTuple(TLVType type, void* value);

unsigned int TLVT_parse(TLVTuple* tlv, byte** buffer);

TLVTuple* TLVT_unparse(byte* buffer, unsigned int* read);


#endif /*_UTILITY_TLV_H_*/
