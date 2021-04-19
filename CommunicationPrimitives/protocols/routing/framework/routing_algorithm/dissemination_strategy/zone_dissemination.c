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

    if(event_type == RTE_NEIGHBORS_CHANGE || event_type == RTE_ANNOUNCE_TIMER || event_type == RTE_SOURCE_EXPIRE) {
        unsigned short radius = *(unsigned short*)(m_state->args);
        BroadcastMessage(msg->Proto_id, radius, 0, (byte*)msg->data, msg->dataLen);
    } else {
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
        } else if(event_type == RTE_CONTROL_MESSAGE) {
            line = true;

            SourceEntry* entry = ((void**)info)[0];

            destination_id = SE_getID(entry);
        } else {
            assert(false);
        }

        printf("EVENT TYPE: %d %d %d\n", event_type, RTE_ROUTE_NOT_FOUND, RTE_CONTROL_MESSAGE);

        if( line ) {
            char str[UUID_STR_LEN];
            uuid_unparse(destination_id, str);
            printf("USING ROUTING TO DISSEMINATE to %s!!!  proto = %d\n", str, msg->Proto_id);
            //BroadcastMessage(msg->Proto_id, 1, (byte*)msg->data, msg->dataLen);
            RouteMessage(destination_id, msg->Proto_id, -1, true, (byte*)msg->data, msg->dataLen);
        } else {
            unsigned int radius = -1; // Infinite
            BroadcastMessage(msg->Proto_id, radius, 1, (byte*)msg->data, msg->dataLen);
        }
    }

}

DisseminationStrategy* ZoneDissemination(unsigned short zone_radius) {

    unsigned short* zone_radius_arg = malloc(sizeof(unsigned short));
    *zone_radius_arg = zone_radius;

    return newDisseminationStrategy(
        zone_radius_arg,
        NULL,
        &disseminate,
        NULL
    );
}
