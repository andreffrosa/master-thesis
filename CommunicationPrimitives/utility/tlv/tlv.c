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

#include "tlv_private.h"
#include "tlv_attrs.h"

#include "utility/my_misc.h"

#include <assert.h>

// assert(TLV_COUNT <= 256);


TLVMessage* newTLVMessage() {

    TLVMessage* tlv_msg = malloc(sizeof(TLVMessage));

    tlv_msg->ht = hash_table_init((hashing_function)&int_hash, (comparator_function)&equalInt);

    return tlv_msg;
}

void destroyTLVMessage(TLVMessage* tlv_msg) {
    if(tlv_msg) {
        // TODO: how to free the tuples? --> tem de ser de acordo com o tipo
    }
}

void TLVM_add(TLVMessage* tlv_msg, TLVTuple* tlv) {
    assert(tlv_msg && tlv);

    void* aux = hash_table_insert(tlv_msg->ht, &tlv->type, tlv);
    assert(aux == NULL);
}

TLVTuple* TLVM_get(TLVMessage* tlv_msg, TLVType type) {
    assert(tlv_msg);

    return (TLVTuple*)hash_table_find_value(tlv_msg->ht, &type);
}

TLVTuple* TLVM_remove(TLVMessage* tlv_msg, TLVType type) {
    assert(tlv_msg);

    hash_table_item* it = hash_table_remove_item(tlv_msg->ht, &type);
    if(it) {
        TLVTuple* tlv = (TLVTuple*)it->value;
        free(it);
        return tlv;
    } else {
        return NULL;
    }
}

TLVTuple* TLVM_next(TLVMessage* tlv_msg, void** iterator) {
    assert(tlv_msg);

    hash_table_item* item = hash_table_iterator_next(tlv_msg->ht, iterator);
    if(item) {
        return (TLVTuple*)(item->value);
    } else {
        return NULL;
    }
}

unsigned short TLVM_parse(TLVMessage* tlv_msg, byte** buffer) {
    assert(tlv_msg && buffer == NULL);

    unsigned short size = 0;
    byte* buffer_ = malloc(TLV_MESSAGE_MAX_SIZE*sizeof(byte));
    byte* ptr = buffer_;

    void* iterator = NULL;
    TLVTuple* tlv = NULL;
    while( (tlv = TLVM_next(tlv_msg, &iterator)) ) {
        byte* aux = NULL;
        unsigned int length = TLVT_parse(tlv, &aux);
        assert(aux);

        memcpy(ptr, aux, length);
        ptr += length;

        free(aux);
    }

    *buffer = buffer_;
    return size;
}

TLVMessage* TLVM_unparse(byte* buffer, unsigned short size) {
    assert(buffer);

    TLVMessage* tlv_msg = newTLVMessage();

    byte* ptr = buffer;

    unsigned int read = 0;
    while( read < size ) {
        unsigned int aux = 0;
        TLVTuple* tlv = TLVT_unparse(ptr, &aux);
        read += aux;
        ptr += aux;

        TLVM_add(tlv_msg, tlv);
    }

    return tlv_msg;
}

TLVTuple* newTLVTuple(TLVType type, void* value) {
    assert(value);

    TLVTuple* tlv = malloc(sizeof(TLVTuple));

    tlv->type = type;
    tlv->value = value;

    return tlv;
}

unsigned int TLVT_parse(TLVTuple* tlv, byte** buffer) {

    const TLVTypeAttrs* attrs = &tlv_type[tlv->type];

    unsigned int buffer_length = sizeof(byte) + attrs->length_size*sizeof(byte) + (attrs->length_size*(MAX_BYTE_VALUE+1) - 1);
    byte* buffer_ = malloc(buffer_length);
    byte* ptr = buffer_;

    // Insert type
    byte type = tlv->type;
    memcpy(ptr, &type, sizeof(byte));
    ptr += sizeof(byte);

    assert(attrs->length_size > 0 && attrs->length_size <= 2);
    unsigned int length = attrs->parse(tlv, ptr + attrs->length_size);

    // Insert length
    if(attrs->length_size == 1) {
        assert(length <= MAX_BYTE_VALUE);
        byte length_ = length;
        memcpy(ptr, &length_, sizeof(byte));
        ptr += sizeof(byte);
    } else if(attrs->length_size == 2) {
        assert(length <= 2*MAX_BYTE_VALUE);
        unsigned short length_ = length;
        memcpy(ptr, &length_, sizeof(unsigned short));
        ptr += sizeof(unsigned short);
    }

    unsigned int total_length = sizeof(byte) + attrs->length_size*sizeof(byte) + length*sizeof(byte);
    return total_length;
}

TLVTuple* TLVT_unparse(byte* buffer, unsigned int* read) {
    assert(buffer);

    byte* ptr = buffer;

    byte type_ = 0;
    memcpy(&type_, ptr, sizeof(byte));
    ptr += sizeof(byte);

    TLVType type = type_;
    const TLVTypeAttrs* attrs = &tlv_type[type];

    assert(attrs->length_size > 0 && attrs->length_size <= 2);

    unsigned int length = 0;

    // Insert length
    if(attrs->length_size == 1) {
        byte length_ = 0;
        memcpy(&length_, ptr, sizeof(byte));
        ptr += sizeof(byte);

        length = length_;
    } else if(attrs->length_size == 2) {
        unsigned short length_ = 0;
        memcpy(&length_, ptr, sizeof(unsigned short));
        ptr += sizeof(unsigned short);

        length = length_;
    }

    byte aux[length];
    memcpy(aux, ptr, length);
    ptr += length;

    *read = sizeof(byte) + attrs->length_size*sizeof(byte) + length*sizeof(byte);
    return attrs->unparse(aux, length);
}
