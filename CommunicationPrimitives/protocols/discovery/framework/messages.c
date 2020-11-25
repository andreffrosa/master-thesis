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

#include "messages.h"

#include "framework.h"

#include <assert.h>

void initHelloMessage(HelloMessage* hello, unsigned char* process_id, unsigned short seq, byte period, float traffic, bool request_replies) {
    assert(hello);

    uuid_copy(hello->process_id, process_id);
    hello->seq = seq;
    hello->period = period;
    hello->traffic = traffic;
    hello->request_replies = request_replies;
}

void initHackMessage(HackMessage* hack, unsigned char* src_process_id, unsigned char* dest_process_id, unsigned short seq, double rx_lq, double tx_lq, byte period, float traffic, byte neigh_type) {
    assert(hack);

    uuid_copy(hack->src_process_id, src_process_id);
    uuid_copy(hack->dest_process_id, dest_process_id);
    hack->seq = seq;
    hack->rx_lq = rx_lq;
    hack->tx_lq = tx_lq;
    hack->period = period;
    hack->traffic = traffic;
    hack->neigh_type = neigh_type;
}
