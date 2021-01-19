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

#include "dissemination_strategy_private.h"

#include "protocols/broadcast/framework/framework.h"
#include "protocols/routing/framework/framework.h"

#include <assert.h>

static void disseminate(ModuleState* m_state, unsigned char* myID, YggMessage* msg, RoutingEventType event_type, void* info) {

    bool line = false;

    unsigned char* destination_id = NULL;

    if(event_type == RTE_ROUTE_NOT_FOUND) {
        assert(info);
        RoutingHeader* header2 = info;

        // Send RERR
        if( uuid_compare(header2->source_id, myID) != 0 ) {
            line = true;

            destination_id = header2->source_id;
        }
    } else if(event_type == RTE_REPLY) {
        line = true;

        SourceEntry* entry = ((void**)info)[0];

        destination_id = SE_getID(entry);
    }

    if( line ) {
        //BroadcastMessage(msg->Proto_id, 1, (byte*)msg->data, msg->dataLen);
        RouteMessage(destination_id, msg->Proto_id, -1, true, (byte*)msg->data, msg->dataLen);
    } else {
        unsigned int radius = -1; // Infinite
        BroadcastMessage(msg->Proto_id, radius, (byte*)msg->data, msg->dataLen);
    }

}

DisseminationStrategy* AODVDissemination() {
    return newDisseminationStrategy(
        NULL,
        NULL,
        &disseminate,
        NULL
    );
}
