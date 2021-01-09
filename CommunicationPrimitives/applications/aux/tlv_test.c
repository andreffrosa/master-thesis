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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uuid/uuid.h>

#include "utility/tlv/tlv.h"
#include "utility/my_misc.h"

int main(int argc, char* argv[]) {

    TLVMessage* tlv_msg = newTLVMessage();

    uuid_t id;
    uuid_parse("66600666-1001-1001-1001-000000000001", id);
    TLVM_add(tlv_msg, newTLVTuple(TLV_SENDER_ID, new_id(id)));

    byte* buffer = NULL;
    unsigned short length = TLVM_parse(tlv_msg, &buffer);
    assert(length > 0);

    destroyTLVMessage(tlv_msg);

    tlv_msg = TLVM_unparse(buffer, length);

    TLVTuple* sender_id_tlv = TLVM_get(tlv_msg, TLV_SENDER_ID);
    assert(sender_id_tlv);

    uuid_t* sender_id = (uuid_t*)TLVT_getValue(sender_id_tlv);
    assert(sender_id);

    char str[UUID_STR_LEN];
    uuid_unparse(*sender_id, str);
    printf("%s\n", str);

    destroyTLVMessage(tlv_msg);

    free(buffer);

    return 0;
}
