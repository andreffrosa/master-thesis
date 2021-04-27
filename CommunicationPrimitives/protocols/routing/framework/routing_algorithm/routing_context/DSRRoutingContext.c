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

#include "routing_context_private.h"

#include "protocols/discovery/framework/framework.h"
#include "protocols/routing/framework/framework.h"

#include "utility/my_string.h"

#include <assert.h>

typedef enum {
    DSR_RREQ,
    DSR_RREP,
    DSR_RERR
} DSRControlMessageType;

static void ProcessDiscoveryEvent(YggEvent* ev, void* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, const char* proto);

static void addRoute(unsigned char* source_id, unsigned char* parent_id, double cost, unsigned int hops, RoutingNeighbors* neighbors, RoutingTable* routing_table, struct timespec* current_time, const char* proto);

static bool getBestBiParent(RoutingNeighbors* neighbors, byte* meta_data, unsigned int meta_length, unsigned char* found_parent, double* found_route_cost, unsigned int* found_route_hops, list** found_route);


static RoutingContextSendType DSRRoutingContextTriggerEvent(ModuleState* m_state, const char* proto, RoutingEventType event_type, void* args, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time) {

    // TODO: verificar se quando é not found faz rreq ou rrer. como é que o aodv faz? envia rrer ao falhar um neigh.

    // aqui como não há acks, deveria enviar rrer quando o next_hop que deveria escolher não é mais um neigh. (algo que a forwarding strategy faz)

    // Aqui era preciso identificar qual das duas coisas ocurria (rreq ou rrer).
    // como? os args estão a ser usados para quê no caso de not found? --> destination id
    // secalhar pode-se introduzir um novo erro que não é route not found mas sim transmission error. mas neste caso não faz sentido porque não estão a ser usados os acks. não interessa porque o erro é não consegiur fazer forward e não não saber a quem enviar.
    // fazer novo erro. acrescentar esse erro ao aodv também. esse erro deveria ser triggered pelos acks mas está a ser triggered por não conhecer o neigh que vai dar ao mesmo em termos de lógica. mas tem o problema de msgs serem enviadas entre o lin quebrar e a deteção do erro. --> não é problema disto

    if(event_type == RTE_ROUTE_NOT_FOUND) {
        //printf("SENDING RREQ\n");
        return SEND_INC;
    } else if(event_type == RTE_NEIGHBORS_CHANGE) {
        YggEvent* ev = args;
        ProcessDiscoveryEvent(ev, NULL, routing_table, neighbors, source_table, myID, current_time, proto);
        //return SEND_INC;
    } else if(event_type == RTE_SOURCE_EXPIRE) {
        SourceEntry* se = (SourceEntry*)args;
        list* route = (list*)SE_remAttr(se, "route");
        list_delete(route);

        // Remove
        list* to_remove = list_init();
        list_add_item_to_tail(to_remove, new_id(SE_getID(se)));
        RF_updateRoutingTable(routing_table, NULL, to_remove, current_time);
    }

    return NO_SEND;
}

static void DSRRoutingContextCreateMsg(ModuleState* m_state, const char* proto, RoutingControlHeader* header, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, YggMessage* msg, RoutingEventType event_type, void* info) {

    if( event_type == RTE_ROUTE_NOT_FOUND ) {
        assert(info);
        RoutingHeader* header2 = info;

        byte type = 0;

        // Send RREQ
        if( uuid_compare(header2->source_id, myID) == 0 ) {
            type = DSR_RREQ;

            YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

            YggMessage_addPayload(msg, (char*)header2->destination_id, sizeof(uuid_t));

            char str[UUID_STR_LEN];
            uuid_unparse(header2->destination_id, str);
            printf("GENERATING REQ to %s\n", str);
        }
        // Send RERR
        else {
            type = DSR_RERR;

            YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

            byte amount = 1;
            YggMessage_addPayload(msg, (char*)&amount, sizeof(byte));

            YggMessage_addPayload(msg, (char*)header2->destination_id, sizeof(uuid_t));

            char str[UUID_STR_LEN];
            uuid_unparse(header2->destination_id, str);
            printf("GENERATING RERR to remove %s\n", str);
        }

    } /*else if(event_type == RTE_NEIGHBORS_CHANGE) {

    }*/ else if(event_type == RTE_CONTROL_MESSAGE) {
        assert(info);
        //SourceEntry* entry = ((void**)info)[0];
        byte* payload = ((void**)info)[1];
        unsigned short length = *((unsigned short*)(((void**)info)[2]));

        RoutingHeader* old_header = (RoutingHeader*)(((void**)info)[3]);

        byte* ptr = payload;

        byte msg_type = 0;
        memcpy(&msg_type, ptr, sizeof(byte));
        ptr += sizeof(byte);

        if(msg_type == DSR_RREP || msg_type == DSR_RREQ) {
            byte type = DSR_RREP;
            YggMessage_addPayload(msg, (char*)&type, sizeof(byte));

            double route_cost = 0.0;
            byte route_hops = 0;

            if( msg_type == DSR_RREP ) {
                /*RoutingTableEntry* rt_entry = RT_findEntry(routing_table, SE_getID(entry)); // The source of the rrep
                route_cost = RTE_getCost(rt_entry);
                route_hops = RTE_getHops(rt_entry);*/

                memcpy(&route_cost, ptr, sizeof(double));
                ptr += sizeof(double);

                memcpy(&route_hops, ptr, sizeof(byte));
                ptr += sizeof(byte);

                RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, old_header->prev_hop_id);

                if(neigh) {
                    double tx_cost = RNE_getTxCost(neigh);

                    route_cost += tx_cost;
                    route_hops += 1;
                } else {
                    route_cost = -1;
                    route_hops = -1;
                }
            }

            YggMessage_addPayload(msg, (char*)&route_cost, sizeof(double));
            YggMessage_addPayload(msg, (char*)&route_hops, sizeof(byte));

            char str[UUID_STR_LEN];
            uuid_unparse(old_header->source_id, str);
            printf("GENERATING RREP to %s \n", str);

        } else if(msg_type == DSR_RERR) {
            YggMessage_addPayload(msg, (char*)payload, length);
        }
    }
}

static RoutingContextSendType DSRRoutingContextProcessMsg(ModuleState* m_state, const char* proto, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, SourceEntry* source_entry, unsigned char* myID, struct timespec* current_time, RoutingControlHeader* header, byte* payload, unsigned short length, unsigned short src_proto, byte* meta_data, unsigned int meta_length) {

    byte* ptr = payload;

    byte type = 0;
    memcpy(&type, ptr, sizeof(byte));
    ptr += sizeof(byte);

    printf("RECEIVED DSR TYPE: %d\n", type);

    if( type == DSR_RREQ ) {
        uuid_t destination_id;
        memcpy(destination_id, ptr, sizeof(uuid_t));
        ptr += sizeof(uuid_t);

        uuid_t found_parent = {0};
        double found_route_cost = 0.0;
        unsigned int found_route_hops = 0;
        list* found_route = NULL;
        bool found = getBestBiParent(neighbors, meta_data, meta_length, found_parent, &found_route_cost, &found_route_hops, &found_route);
        if( found ) {

            if( uuid_compare(destination_id, myID) == 0 ) {

                printf("I'm the wanted destination! SEND RREP\n");

                list_add_item_to_tail(found_route, new_id(myID));
                list* to_src_route = reverse(found_route, sizeof(uuid_t));

                printf("Found route n=%d\n", to_src_route->size);

                for(list_item* it = to_src_route->head; it; it = it->next) {
                    char id_str[UUID_STR_LEN];
                    uuid_unparse(it->data, id_str);
                    printf(" => %s\n", id_str);
                }

                list* old = (list*)SE_setAttr(source_entry, new_str("route"), to_src_route);
                if(old) {
                    list_delete(old);
                }

                // Update Routing Table
                addRoute(SE_getID(source_entry), found_parent, found_route_cost, found_route_hops, neighbors, routing_table, current_time, proto);

                return SEND_INC; // SEND RREP
            } else {
                list_add_item_to_tail(found_route, new_id(myID));
                list* to_src_route = reverse(found_route, sizeof(uuid_t));

                printf("Found route n=%d\n", to_src_route->size);

                for(list_item* it = to_src_route->head; it; it = it->next) {
                    char id_str[UUID_STR_LEN];
                    uuid_unparse(it->data, id_str);
                    printf(" => %s\n", id_str);
                }

                list* old = (list*)SE_setAttr(source_entry, new_str("route"), to_src_route);
                if(old) {
                    list_delete(old);
                }

                // Update Routing Table
                addRoute(SE_getID(source_entry), found_parent, found_route_cost, found_route_hops, neighbors, routing_table, current_time, proto);

            }
        }
    } else if( type == DSR_RREP ) {

        char str[UUID_STR_LEN];
        uuid_unparse(SE_getID(source_entry), str);
        printf("RECEIVED RREP with route to %s\n", str);

        uuid_t destination_id;
        uuid_t prev_hop_id;

        list* route = list_init();

        if(src_proto == ROUTING_FRAMEWORK_PROTO_ID) {
            RoutingHeader* header = (RoutingHeader*)((void**)meta_data)[0];
            byte* prev_meta_data = (byte*)((void**)meta_data)[1];
            //bool first

            uuid_copy(destination_id, header->destination_id);
            uuid_copy(prev_hop_id, header->prev_hop_id);

            unsigned int n = header->meta_data_length / sizeof(uuid_t);
            for(int i = 0; i < n; i++) {
                unsigned char* id = prev_meta_data + (i * sizeof(uuid_t));
                list_add_item_to_head(route, new_id(id));
            }

            printf("Route contained in RREP (%d):\n", route->size);
            for(list_item* it = route->head; it; it = it->next) {
                char id_str[UUID_STR_LEN];
                uuid_unparse(it->data, id_str);
                printf(" => %s\n", id_str);
            }
        } else {
            printf("Ya!\n");
            assert(false);
        }

        double route_cost = 0.0;
        byte route_hops = 0;

        memcpy(&route_cost, ptr, sizeof(double));
        ptr += sizeof(double);

        memcpy(&route_hops, ptr, sizeof(byte));
        ptr += sizeof(byte);


        //if(uuid_compare(myID, destination_id) == 0) {
            RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, prev_hop_id);
            if(neigh && RNE_isBi(neigh)) {
                route_cost += RNE_getTxCost(neigh);
                route_hops += 1;

                list* old = (list*)SE_setAttr(source_entry, new_str("route"), route);
                if(old) {
                    list_delete(old);
                }

                // Update Routing Table
                addRoute(SE_getID(source_entry), prev_hop_id, route_cost, route_hops, neighbors, routing_table, current_time, proto);
            } else {
                list_delete(route);
            }
        //} else {
        //    list_delete(route);
        //}

    }
    else if( type == DSR_RERR ) {

        uuid_t dest_to_remove_id;
        uuid_t prev_hop_id;
        //uuid_t next_hop_id;

        if(src_proto == ROUTING_FRAMEWORK_PROTO_ID) {
            RoutingHeader* header = (RoutingHeader*)meta_data;

            uuid_copy(prev_hop_id, header->prev_hop_id);
        } else {
            assert(false);
        }

        byte amount = 0;
        memcpy(&amount, ptr, sizeof(byte));
        ptr += sizeof(byte);

        // Update Routing Table
        list* to_remove = list_init();

        for(int i = 0; i < amount; i++) {
            memcpy(dest_to_remove_id, ptr, sizeof(uuid_t));
            ptr += sizeof(uuid_t);

            /*byte has_seq = false;
            memcpy(&has_seq, ptr, sizeof(byte));
            ptr += sizeof(byte);*/


            /*
            unsigned short seq = 0;
            if(has_seq) {
                memcpy(&seq, ptr, sizeof(unsigned short));
                ptr += sizeof(unsigned short);*/

                /*SourceEntry* entry2 = ST_getEntry(source_table, dest_to_remove_id);
                if( entry2 && compare_seq(unsigned short s1, unsigned short s2, bool ignore_zero)   ) {
                    list_add_item_to_tail(to_remove, new_id(dest_to_remove_id));
                }*/
                /*list_add_item_to_tail(to_remove, new_id(dest_to_remove_id));
            } else {*/
                //list_add_item_to_tail(to_remove, new_id(dest_to_remove_id));
            //}

            RoutingTableEntry* rt_entry = RT_findEntry(routing_table, dest_to_remove_id);
            if(rt_entry && strcmp(RTE_getProto(rt_entry), proto) == 0) {
                list_add_item_to_tail(to_remove, new_id(dest_to_remove_id));
            }

            char str[UUID_STR_LEN];
            uuid_unparse(dest_to_remove_id, str);
            printf("RECEIVED RERR to %s\n", str);
        }

        RF_updateRoutingTable(routing_table, NULL, to_remove, current_time);
    }

    return NO_SEND;
}

//typedef void (*rc_destroy)(ModuleState* m_state);

RoutingContext* DSRRoutingContext() {

    return newRoutingContext(
        "DSR",
        NULL,
        NULL,
        NULL, // &DSRRoutingContextInit,
        &DSRRoutingContextTriggerEvent,
        &DSRRoutingContextCreateMsg,
        &DSRRoutingContextProcessMsg,
        NULL
    );
}

static bool getBestBiParent(RoutingNeighbors* neighbors, byte* meta_data, unsigned int meta_length, unsigned char* found_parent, double* found_route_cost, unsigned int* found_route_hops, list** found_route) {

    uuid_t min_parent_id = {0};
    double min_route_cost = 0.0;
    unsigned int min_route_hops = 0;
    list* min_route = NULL;
    bool first = true;


    byte* ptr2 = meta_data;

    uuid_t source_id;
    memcpy(source_id, ptr2, sizeof(uuid_t));
    ptr2 += sizeof(uuid_t);

    byte n_copies = 0;
    memcpy(&n_copies, ptr2, sizeof(byte));
    ptr2 += sizeof(byte);

    for(int i = 0; i < n_copies; i++) {
        uuid_t parent_id;
        memcpy(parent_id, ptr2, sizeof(uuid_t));
        ptr2 += sizeof(uuid_t);

        ptr2 += sizeof(struct timespec);

        unsigned short context_length = 0;
        memcpy(&context_length, ptr2, sizeof(unsigned short));
        ptr2 += sizeof(unsigned short);

        byte context[context_length];
        memcpy(context, ptr2, context_length);
        ptr2 += context_length;

        // parse context
        double route_cost = 0.0;
        unsigned int route_hops = 0;
        list* route = NULL;
        bool has_cost = false;
        bool has_hops = false;
        bool has_route = false;

        byte* ptr3 = context;
        unsigned int read = 0;
        while( read < context_length ) {
            char key[101];
            unsigned int key_len = strlen((char*)ptr3)+1;
            memcpy(key, ptr3, key_len);
            ptr3 += key_len;
            read += key_len;

            byte len = 0;
            memcpy(&len, ptr3, sizeof(byte));
            ptr3 += sizeof(byte);
            read += sizeof(byte);

            byte value[len];
            memcpy(value, ptr3, len);
            ptr3 += len;
            read += len;

            if(strcmp(key, "route_cost") == 0) {
                assert(len == sizeof(double));
                route_cost = *((double*)value);

                // add cost do parent
                /*RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, parent_id);
                assert(neigh);
                double tx_cost = RNE_getTxCost(neigh);
                route_cost += tx_cost;*/

                printf("context: %s -> (%u bytes, %f)\n", key, len, route_cost);
                has_cost = true;
            } else if(strcmp(key, "hops") == 0) {
                assert(len == sizeof(byte));
                route_hops = *((byte*)value);
                printf("context: %s -> (%u bytes, %u)\n", key, len, route_hops);
                has_hops = true;
            } else if(strcmp(key, "route") == 0) {
                route = list_init();
                byte* ptr4 = value;
                int n = len / sizeof(uuid_t);
                for(int i = 0; i < n; i++) {
                    list_add_item_to_tail(route, new_id(ptr4));
                    ptr4 += sizeof(uuid_t);
                }
                printf("context: %s -> (%u bytes, %u)\n", key, len, route_hops);
                has_route = true;

            } else {
                // debug
                printf("context: %s -> (%u bytes, - )\n", key, len);
            }
        }

        assert(route_hops == route->size);

        RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, parent_id);
        if( neigh && RNE_isBi(neigh) && has_cost && has_hops && has_route ) { // parent is bi
            route_cost += RNE_getTxCost(neigh);
            //route_hops += 1;

            if( route_cost < min_route_cost || (route_cost == min_route_cost && route_hops < min_route_hops) || first ) {
                first = false;
                min_route_cost = route_cost;
                min_route_hops = route_hops;
                min_route = list_clone(route, sizeof(uuid_t));
                uuid_copy(min_parent_id, parent_id);
            }
        }

        list_delete(route);
    }

    if(!first) {
        *found_route_cost = min_route_cost;
        *found_route_hops = min_route_hops;
        *found_route = min_route;
        uuid_copy(found_parent, min_parent_id);
        return true;
    } else {
        return false;
    }

}


static void addRoute(unsigned char* source_id, unsigned char* parent_id, double cost, unsigned int hops, RoutingNeighbors* neighbors, RoutingTable* routing_table, struct timespec* current_time, const char* proto) {

    RoutingTableEntry* old_entry = RT_findEntry(routing_table, source_id);

    // bool update = old_entry ? ( strcmp(RTE_getProto(old_entry), proto) != 0 ? false : (uuid_compare(RTE_getNextHopID(old_entry), parent_id) == 0 ? !(RTE_getCost(old_entry) == cost && RTE_getHops(old_entry) == hops) : true )) : true;

    bool update = old_entry ? (uuid_compare(RTE_getNextHopID(old_entry), parent_id) == 0 ? !(RTE_getCost(old_entry) == cost && RTE_getHops(old_entry) == hops) : true ) : true;

    if(update) {
        list* to_update = list_init();

        RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, parent_id);
        assert(neigh);
        WLANAddr* addr = RNE_getAddr(neigh);

        RoutingTableEntry* new_entry = newRoutingTableEntry(source_id, parent_id, addr, cost, hops, current_time, proto);

        list_add_item_to_tail(to_update, new_entry);

        RF_updateRoutingTable(routing_table, to_update, NULL, current_time);
    }

}

static void ProcessDiscoveryEvent(YggEvent* ev, void* state, RoutingTable* routing_table, RoutingNeighbors* neighbors, SourceTable* source_table, unsigned char* myID, struct timespec* current_time, const char* proto) {
    assert(ev);

    unsigned short ev_id = ev->notification_id;

    bool process = ev_id == NEW_NEIGHBOR || ev_id == UPDATE_NEIGHBOR || ev_id == LOST_NEIGHBOR;
    if(process) {
        unsigned short read = 0;
        unsigned char* ptr = ev->payload;

        unsigned short length = 0;
        memcpy(&length, ptr, sizeof(unsigned short));
        ptr += sizeof(unsigned short);
        read += sizeof(unsigned short);

        YggEvent main_ev = {0};
        YggEvent_init(&main_ev, DISCOVERY_FRAMEWORK_PROTO_ID, 0);
        YggEvent_addPayload(&main_ev, ptr, length);
        ptr += length;
        read += length;

        unsigned char* ptr2 = NULL;

        uuid_t id;
        ptr2 = YggEvent_readPayload(&main_ev, ptr2, id, sizeof(uuid_t));

        bool remove = false;

        if(ev_id == NEW_NEIGHBOR || ev_id == UPDATE_NEIGHBOR) {
            RoutingNeighborsEntry* neigh = RN_getNeighbor(neighbors, id);
            assert(neigh);

            if( RNE_isBi(neigh) ) {
                double cost = RNE_getTxCost(neigh);
                addRoute(id, id, cost, 1, neighbors, routing_table, current_time, proto);
            } else {
                remove = true;
            }
        } else {
            remove = true;
        }

        if(remove) {
            list* to_remove = list_init();

            //list_add_item_to_tail(to_remove, new_id(id));

            void* iterator = NULL;
            RoutingTableEntry* current_route = NULL;
            while( (current_route = RT_nextRoute(routing_table, &iterator)) ) {
                if( uuid_compare(RTE_getNextHopID(current_route), id) == 0) {
                    list_add_item_to_tail(to_remove, new_id(RTE_getDestinationID(current_route)));
                }
            }

            RF_updateRoutingTable(routing_table, NULL, to_remove, current_time);
        }
    }
}
