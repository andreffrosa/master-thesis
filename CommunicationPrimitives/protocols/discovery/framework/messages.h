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

#ifndef PROTOCOLS_DISCOVERY_MESSAGES_H_
#define PROTOCOLS_DISCOVERY_MESSAGES_H_

#include <uuid/uuid.h>

#include "utility/byte.h"

#include "Yggdrasil.h"

#pragma pack(1)
typedef struct _HelloMessage {
    uuid_t process_id;
    unsigned short seq;
    byte period;
    float traffic;
    bool request_replies;
} HelloMessage;
#pragma pack()

#pragma pack(1)
typedef struct _HackMessage {
    uuid_t src_process_id;
    uuid_t dest_process_id;
    unsigned short seq;
    float rx_lq;
    float tx_lq;
    byte period;
    float traffic;
    byte neigh_type;
} HackMessage;
#pragma pack()

void initHelloMessage(HelloMessage* hello, unsigned char* process_id, unsigned short seq, byte period, float traffic, bool request_replies);

void initHackMessage(HackMessage* hack, unsigned char* src_process_id, unsigned char* dest_process_id, unsigned short seq, double rx_lq, double tx_lq, byte period, float traffic, byte neigh_type);

#endif /* PROTOCOLS_DISCOVERY_MESSAGES_H_ */
