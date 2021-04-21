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
 * (C) 2019
 *********************************************************/

#include "retransmission_context_private.h"

#include "utility/my_math.h"
#include "utility/my_string.h"

//#include "protocols/routing/framework/framework.h"
#define ROUTING_FRAMEWORK_PROTO_ID 161
#define EV_ROUTING_NEIGHS 0

#include <assert.h>

typedef struct RoutingNeigh_ {
    double rx_cost;
    double tx_cost;
    bool is_bi;
} RoutingNeigh;

typedef struct BiFloodingState_ {
    hash_table* neighbors;
} BiFloodingState;

static bool getCost(BiFloodingState* state, double_list* copies, byte* hops, double* cost, list** route, unsigned char* myID) {
    assert(copies && copies->size > 0);
    assert(route);

    double min_cost = 0.0;
    unsigned int min_hops = 0;
    list* min_route = NULL;
    bool first = true;

    for(double_list_item* dit = copies->head; dit; dit = dit->next) {
        MessageCopy* copy = (MessageCopy*)dit->data;
        hash_table* headers = getHeaders(copy);

        byte* hops_ = (byte*)hash_table_find_value(headers, "hops");
        double* cost_ = (double*)hash_table_find_value(headers, "route_cost");
        list* route_ = (list*)hash_table_find_value(headers, "route");

        if(hops_ && cost_ && route_) {
            assert(*hops_ == route_->size);

            RoutingNeigh* n = hash_table_find_value(state->neighbors, getBcastHeader(copy)->sender_id);

            printf("Parent: %s\n", (n?"T":"F"));

            if(n && n->is_bi) {
                double current_cost = *cost_ + n->tx_cost;
                unsigned int current_hops = *hops_; // + 1;

                if( current_cost < min_cost || (current_cost == min_cost && current_hops < min_hops) || first ) {
                    min_cost = current_cost;
                    min_hops = current_hops;
                    list_delete(min_route);
                    min_route = list_clone(route_, sizeof(uuid_t));
                    first = false;
                }
            }
        }

        //list_delete(route_);
    }

    if(min_route == NULL) {
        min_route = list_init();
    }
    //list_add_item_to_tail(min_route, new_id(myID));

    *cost = min_cost;
    *hops = min_hops;
    *route = min_route;

    assert(*route != NULL);
    assert(*hops == (*route)->size);

    return !first;
}


static void BiFloodingContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID) {
    proto_def_add_consumed_event(protocol_definition, ROUTING_FRAMEWORK_PROTO_ID, EV_ROUTING_NEIGHS);
}

static void BiFloodingContextEvent(ModuleState* context_state, queue_t_elem* elem, unsigned char* myID, hash_table* contexts) {
    BiFloodingState* state = (BiFloodingState*) (context_state->vars);

    if(elem->type == YGG_EVENT) {
        YggEvent* ev = &elem->data.event;

        if( ev->proto_origin == ROUTING_FRAMEWORK_PROTO_ID ) {
            unsigned short ev_id = ev->notification_id;

            bool process = ev_id == EV_ROUTING_NEIGHS;
            if(process) {

                // printf("EV_ROUTING_NEIGHS\n");

                hash_table_delete(state->neighbors);
                state->neighbors = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

                unsigned short read = 0;
                unsigned char* ptr = ev->payload;

                byte amount = 0;
                memcpy(&amount, ptr, sizeof(byte));
            	ptr += sizeof(byte);
                read += sizeof(byte);

                for(int i = 0; i < amount; i++) {
                    uuid_t id;
                    memcpy(id, ptr, sizeof(uuid_t));
                    ptr += sizeof(uuid_t);
                    read += sizeof(uuid_t);

                    ptr += WLAN_ADDR_LEN;
                    read += WLAN_ADDR_LEN;

                    double rx_cost = 0.0;
                    memcpy(&rx_cost, ptr, sizeof(double));
                    ptr += sizeof(double);
                    read += sizeof(double);

                    double tx_cost = 0.0;
                    memcpy(&tx_cost, ptr, sizeof(double));
                    ptr += sizeof(double);
                    read += sizeof(double);

                    byte is_bi = false;
                    memcpy(&is_bi, ptr, sizeof(byte));
                    ptr += sizeof(byte);
                    read += sizeof(byte);

                    ptr += sizeof(struct timespec);
                    read += sizeof(struct timespec);

                    RoutingNeigh* n = malloc(sizeof(RoutingNeigh));
                    n->rx_cost = rx_cost;
                    n->tx_cost = tx_cost;
                    n->is_bi = is_bi;
                    hash_table_insert(state->neighbors, new_id(id), n);
                }
            }
        }
    }
}

static bool BiFloodingContextQuery(ModuleState* context_state, const char* query, void* result, hash_table* query_args, unsigned char* myID, hash_table* contexts) {
	BiFloodingState* state = (BiFloodingState*) (context_state->vars);

	if(strcmp(query, "bi") == 0) {
        assert(query_args);
        void** aux1 = hash_table_find_value(query_args, "p_msg");
        assert(aux1);
		PendingMessage* p_msg = *aux1;
        assert(p_msg);

        byte hops = 0;
        double route_cost = 0.0;
        list* route = NULL;
        *((bool*)result) = getCost(state, getCopies(p_msg), &hops, &route_cost, &route, myID);
        list_delete(route);
		return true;
    } else {
		return false;
	}
}

static void BiFloodingContextAppendHeaders(ModuleState* context_state, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time) {

    BiFloodingState* state = (BiFloodingState*) (context_state->vars);

    byte hops = 0;
    double route_cost = 0.0;
    list* route = NULL;

    getCost(state, getCopies(p_msg), &hops, &route_cost, &route, myID);
    hops++;
    list_add_item_to_tail(route, new_id(myID));
    assert(hops == route->size);

    unsigned int size = route->size*sizeof(uuid_t);
    byte buffer[size];
    byte* ptr = buffer;

    for(list_item* it = route->head; it; it = it->next) {
        unsigned char* id = (unsigned char*)(it->data);
        memcpy(ptr, id, sizeof(uuid_t));
        ptr += sizeof(uuid_t);
    }
    list_delete(route);

    appendHeader(serialized_headers, "hops", &hops, sizeof(byte));
    appendHeader(serialized_headers, "route_cost", &route_cost, sizeof(double));
    appendHeader(serialized_headers, "route", buffer, size);
}

static void BiFloodingContextParseHeaders(ModuleState* context_state, hash_table* serialized_headers, hash_table* headers, unsigned char* myID) {

    byte* buffer = (byte*)hash_table_find_value(serialized_headers, "hops");
    if(buffer) {
        byte* ptr = buffer + sizeof(byte);

        byte* hops = malloc(sizeof(byte));
        memcpy(hops, ptr, sizeof(byte));

        const char* key_ = "hops";
        char* key = malloc(strlen(key_)+1);
        strcpy(key, key_);
        hash_table_insert(headers, key, hops);
    }

    buffer = (byte*)hash_table_find_value(serialized_headers, "route_cost");
    if(buffer) {
        byte* ptr = buffer + sizeof(byte);

        double* route_cost = malloc(sizeof(double));
        memcpy(route_cost, ptr, sizeof(double));

        const char* key_ = "route_cost";
        char* key = malloc(strlen(key_)+1);
        strcpy(key, key_);
        hash_table_insert(headers, key, route_cost);
    }

    buffer = (byte*)hash_table_find_value(serialized_headers, "route");
    if(buffer) {
        byte* ptr = buffer;

        byte size = 0;
        memcpy(&size, ptr, sizeof(byte));
        ptr += sizeof(byte);

        list* route = list_init();

        int n = size / sizeof(uuid_t);
        for(int i = 0; i < n; i++) {
            list_add_item_to_tail(route, new_id(ptr));
            ptr += sizeof(uuid_t);
        }

        printf("n=%d size=%d route->size=%d\n", n, size, route->size);

        const char* key_ = "route";
        char* key = malloc(strlen(key_)+1);
        strcpy(key, key_);
        hash_table_insert(headers, key, route);
    }

}

static void BiFloodingContextDestroy(ModuleState* context_state) {

    BiFloodingState* state = (BiFloodingState*) (context_state->vars);
    hash_table_delete(state->neighbors);
    free(state);
}

RetransmissionContext* BiFloodingContext() {

    BiFloodingState* state = malloc(sizeof(BiFloodingState));

    state->neighbors = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    return newRetransmissionContext(
        "BiFloodingContext",
        NULL,
        state,
        &BiFloodingContextInit,
        &BiFloodingContextEvent,
        &BiFloodingContextAppendHeaders,
        &BiFloodingContextParseHeaders,
        &BiFloodingContextQuery,
        NULL,
        &BiFloodingContextDestroy,
        NULL
    );
}
