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

#include <assert.h>

static void getCost(double_list* copies, byte* hops, double* cost) {
    assert(copies && copies->size > 0);

    double min_cost = 0.0; 
    unsigned int min_hops = 1;
    bool first = true;

    for(double_list_item* dit = copies->head; dit; dit = dit->next) {
        MessageCopy* copy = (MessageCopy*)dit->data;
        hash_table* headers = getHeaders(copy);

        byte* hops_ = (byte*)hash_table_find_value(headers, "hops");
        double* cost_ = (double*)hash_table_find_value(headers, "route_cost");

        if(hops_ && cost_) {
            double current_cost = *cost_ + 1.0; // TODO: get neigh tx cost;
            unsigned int current_hops = *hops_ + 1;

            if(true) { // TODO: neigh is bi
                if( current_cost < min_cost || (current_cost == min_cost && current_hops < min_hops) || first ) {
                    min_cost = current_cost;
                    min_hops = current_hops;
                    first = false;
                }
            }
        }
    }

    *cost = min_cost;
    *hops = min_hops;
}

static void BiFloodingContextAppendHeaders(ModuleState* context_state, PendingMessage* p_msg, hash_table* serialized_headers, unsigned char* myID, hash_table* contexts, struct timespec* current_time) {

    byte hops = 0;
    double route_cost = 0.0;
    getCost(getCopies(p_msg), &hops, &route_cost);

    appendHeader(serialized_headers, "hops", &hops, sizeof(byte));
    appendHeader(serialized_headers, "route_cost", &route_cost, sizeof(double));
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
}

RetransmissionContext* BiFloodingContext() {

    return newRetransmissionContext(
        "BiFloodingContext",
        NULL,
        NULL,
        NULL,
        NULL,
        &BiFloodingContextAppendHeaders,
        &BiFloodingContextParseHeaders,
        NULL,
        NULL,
        NULL,
        NULL
    );
}
