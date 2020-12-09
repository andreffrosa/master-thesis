
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

#include "discovery_message_private.h"

#include <assert.h>

#include "utility/my_misc.h"
#include "utility/olsr_utils.h"

typedef enum {
    NULL_MPR,
    FLOODING_MPR,
    ROUTING_MPR,
    FLOOD_ROUTE_MPR
} NeighMPRType;

typedef struct OLSRAttrs_ {
    bool flooding_mpr;
    bool flooding_mpr_selector; // L_mpr_selector
    bool routing_mpr;
    bool routing_mpr_selector; // N_mpr_selector
    // bool advertised;
} OLSRAttrs;

typedef struct OLSRState_ {
    list* flooding_mprs;
    list* routing_mprs;
    list* flooding_mpr_selectors;
    list* routing_mpr_selectors;
} OLSRState;

static void* OLSR_createAttrs(ModuleState* state) {
    OLSRAttrs* attrs = malloc(sizeof(OLSRAttrs));

    attrs->flooding_mpr = false;
    attrs->flooding_mpr_selector = false;
    attrs->routing_mpr = false;
    attrs->routing_mpr_selector = false;
    // attrs->advertised = false;

    return attrs;
}

/*
static void OLSR_destroyAttrs(ModuleState* state, void* d_msg_attrs) {

}
*/

static list* compute_mprs(bool broadcast, NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time);
static list* compute_broadcast_mprs(NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time);
static list* compute_routing_mprs(NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time);

static NeighMPRType getMPRType(bool flooding_mpr, bool routing_mpr) {
    if( flooding_mpr && !routing_mpr ) {
        return FLOODING_MPR;
    } else if( !flooding_mpr && routing_mpr ) {
        return ROUTING_MPR;
    } else if( flooding_mpr && routing_mpr ) {
        return FLOOD_ROUTE_MPR;
    } else {
        return NULL_MPR;
    }
}

static bool recompute_mprs(OLSRState* state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors) {

    bool changed = false;

    list* new_flooding_mprs = compute_broadcast_mprs(neighbors, myID, current_time);
    list* new_routing_mprs = compute_routing_mprs(neighbors, myID, current_time);

    // printf("\n\n\t\t\tRECOMPUTE MPRS Flooding %d -> %d Routing %d -> %d \n\n", state->flooding_mprs->size, new_flooding_mprs->size, state->routing_mprs->size, new_routing_mprs->size);

    if( !list_equal(new_flooding_mprs, state->flooding_mprs, &equalID) ) {
        list* old = state->flooding_mprs;
        state->flooding_mprs = new_flooding_mprs;
        list_delete(old);

        changed = true;
    } else {
        list_delete(new_flooding_mprs);
        //changed = false;
    }

    if( !list_equal(new_routing_mprs, state->routing_mprs, &equalID) ) {
        list* old = state->routing_mprs;
        state->routing_mprs = new_routing_mprs;
        list_delete(old);

        changed = true;
    } else {
        list_delete(new_routing_mprs);
        //changed = false;
    }

    if( changed ) {
        // Notify
        unsigned int size = 2*sizeof(unsigned int) + (state->flooding_mprs->size + state->routing_mprs->size)*sizeof(uuid_t);
        byte buffer[size];
        byte* ptr = buffer;

        unsigned int aux = state->flooding_mprs->size;
        memcpy(ptr, &aux, sizeof(unsigned int));
        ptr += sizeof(unsigned int);

        for( list_item* it = state->flooding_mprs->head; it; it = it->next ) {
            unsigned char* id = (unsigned char*)it->data;

            memcpy(ptr, id, sizeof(uuid_t));
            ptr += sizeof(uuid_t);
        }

        aux = state->routing_mprs->size;
        memcpy(ptr, &aux, sizeof(unsigned int));
        ptr += sizeof(unsigned int);

        for( list_item* it = state->routing_mprs->head; it; it = it->next ) {
            unsigned char* id = (unsigned char*)it->data;

            memcpy(ptr, id, sizeof(uuid_t));
            ptr += sizeof(uuid_t);
        }

        DF_notifyGenericEvent("MPRS", buffer, size);

        //////////////////////////////////////


        // Refresh Neighbor's Attributes
        void* iterator = NULL;
        NeighborEntry* current_neigh = NULL;
        OLSRAttrs* neigh_attrs = NULL;
        while( (current_neigh = NT_nextNeighbor(neighbors, &iterator)) ) {
            neigh_attrs = NE_getMessageAttributes(current_neigh);

            if( NE_getNeighborType(current_neigh, current_time) == BI_NEIGH ) {
                neigh_attrs->flooding_mpr = list_find_item(state->flooding_mprs, &equalID, NE_getNeighborID(current_neigh)) != NULL;
                neigh_attrs->routing_mpr = list_find_item(state->routing_mprs, &equalID, NE_getNeighborID(current_neigh)) != NULL;
            } else {
                neigh_attrs->flooding_mpr = false;
                neigh_attrs->routing_mpr = false;
                neigh_attrs->flooding_mpr_selector = false;
                neigh_attrs->routing_mpr_selector = false;
            }
        }
    }

    return changed;
}

static bool OLSR_createMessage(ModuleState* m_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, MessageType msg_type, void* aux_info, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size) {
    assert(hello);

    OLSRState* state = (OLSRState*)m_state->vars;

    // Recompute mprs if necessary
    if( msg_type == NEIGHBOR_CHANGE_MSG ) {
        NeighborChangeSummary* s = (NeighborChangeSummary*)aux_info;

        bool one_hop_change = s->new_neighbor || s->updated_neighbor || s->lost_neighbor;
        bool two_hop_change = s->updated_2hop_neighbor || s->added_2hop_neighbor || s->lost_2hop_neighbor;

        if( one_hop_change || two_hop_change ) {
            bool changed = recompute_mprs(state, myID, current_time, neighbors);

            // if the mprs didn't change and the changes where only at two hop neighbors and is not periodic, then don't send a message
            if( !changed && !one_hop_change && two_hop_change ) {
                return false;
            }
        }
    }

    // Serialize Message

    byte* ptr = buffer;

    // Serialize Hello
    memcpy(ptr, hello, sizeof(HelloMessage));
    ptr += sizeof(HelloMessage);
    *size += sizeof(HelloMessage);

    // Serialize Hacks
    ptr[0] = n_hacks;
    ptr += 1;
    *size += 1;

    for(int i = 0; i < n_hacks; i++) {
        memcpy(ptr, &hacks[i], sizeof(HackMessage));
        ptr += sizeof(HackMessage);
        *size += sizeof(HackMessage);

        NeighborEntry* neigh = NT_getNeighbor(neighbors, hacks[i].dest_process_id);
        assert(neigh);
        OLSRAttrs* neigh_attrs = NE_getMessageAttributes(neigh);

        NeighMPRType mpr_type = getMPRType(neigh_attrs->flooding_mpr, neigh_attrs->routing_mpr);
        byte aux = mpr_type;
        memcpy(ptr, &aux, sizeof(aux));
        ptr += sizeof(aux);
        *size += sizeof(aux);
    }

    return true;
}


static bool OLSR_processMessage(ModuleState* m_state, void* f_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size) {

    OLSRState* state = (OLSRState*)m_state->vars;

    byte* ptr = buffer;

    // Deserialize Hello
    HelloMessage hello;
    memcpy(&hello, ptr, sizeof(HelloMessage));
    ptr += sizeof(HelloMessage);

    HelloDeliverSummary* summary = deliverHello(f_state, &hello, mac_addr);
    free(summary);

    // Deserialize Hacks
    byte n_hacks = ptr[0];
    ptr += 1;

    bool changed_mpr_selectors = false;

    if( n_hacks > 0 ) {
        HackMessage hacks[n_hacks];

        for(int i = 0; i < n_hacks; i++) {
            memcpy(&hacks[i], ptr, sizeof(HackMessage));
            ptr += sizeof(HackMessage);

            HackDeliverSummary* summary = deliverHack(f_state, &hacks[i]);
            free(summary);

            byte aux;
            memcpy(&aux, ptr, sizeof(aux));
            ptr += sizeof(aux);
            NeighMPRType mpr_type = aux;

            if( uuid_compare(hacks[i].dest_process_id, myID) == 0 ) {
                NeighborEntry* neigh = NT_getNeighbor(neighbors, hacks[i].src_process_id);
                assert(neigh);
                OLSRAttrs* neigh_attrs = NE_getMessageAttributes(neigh);

                if( mpr_type == FLOODING_MPR || mpr_type == FLOOD_ROUTE_MPR ) {
                    neigh_attrs->flooding_mpr_selector = true;

                    if( list_find_item(state->flooding_mpr_selectors, &equalID, NE_getNeighborID(neigh)) == NULL ) {
                        void* x = malloc(sizeof(uuid_t));
                        uuid_copy(x, NE_getNeighborID(neigh));
                        list_add_item_to_tail(state->flooding_mpr_selectors, x);

                        changed_mpr_selectors = true;
                    }
                } else {
                    neigh_attrs->flooding_mpr_selector = false;

                    void* aux = list_remove_item(state->flooding_mpr_selectors, &equalID, NE_getNeighborID(neigh));
                    if(aux) {
                        free(aux);
                        changed_mpr_selectors = true;
                    }
                }

                if( mpr_type == ROUTING_MPR || mpr_type == FLOOD_ROUTE_MPR ) {
                    neigh_attrs->routing_mpr_selector = true;

                    if( list_find_item(state->routing_mpr_selectors, &equalID, NE_getNeighborID(neigh)) == NULL ) {
                        void* x = malloc(sizeof(uuid_t));
                        uuid_copy(x, NE_getNeighborID(neigh));
                        list_add_item_to_tail(state->routing_mpr_selectors, x);

                        changed_mpr_selectors = true;
                    }
                } else {
                    neigh_attrs->routing_mpr_selector = false;

                    void* aux = list_remove_item(state->routing_mpr_selectors, &equalID, NE_getNeighborID(neigh));
                    if(aux) {
                        free(aux);
                        changed_mpr_selectors = true;
                    }
                }
            }
        }
    }

    // Notify
    if( changed_mpr_selectors ) {
        unsigned int size = 2*sizeof(unsigned int) + (state->flooding_mpr_selectors->size + state->routing_mpr_selectors->size)*sizeof(uuid_t);
        byte buffer[size];
        byte* ptr = buffer;

        unsigned int aux = state->flooding_mpr_selectors->size;
        memcpy(ptr, &aux, sizeof(unsigned int));
        ptr += sizeof(unsigned int);

        for( list_item* it = state->flooding_mpr_selectors->head; it; it = it->next ) {
            unsigned char* id = (unsigned char*)it->data;

            memcpy(ptr, id, sizeof(uuid_t));
            ptr += sizeof(uuid_t);
        }

        aux = state->routing_mpr_selectors->size;
        memcpy(ptr, &aux, sizeof(unsigned int));
        ptr += sizeof(unsigned int);

        for( list_item* it = state->routing_mpr_selectors->head; it; it = it->next ) {
            unsigned char* id = (unsigned char*)it->data;

            memcpy(ptr, id, sizeof(uuid_t));
            ptr += sizeof(uuid_t);
        }

        DF_notifyGenericEvent("MPR SELECTORS", buffer, size);
    }

    return false;
}

static void OLSR_destructor(ModuleState* m_state) {
    OLSRState* state = (OLSRState*)m_state->vars;

    list_delete(state->flooding_mprs);
    list_delete(state->flooding_mpr_selectors);
    list_delete(state->routing_mprs);
    list_delete(state->routing_mpr_selectors);

    free(state);
}

DiscoveryMessage* OLSRDiscoveryMessage() {
    OLSRState* state = malloc(sizeof(OLSRState));
    state->flooding_mprs = list_init();
    state->flooding_mpr_selectors = list_init();
    state->routing_mprs = list_init();
    state->routing_mpr_selectors = list_init();

    return newDiscoveryMessage(NULL, state, &OLSR_createMessage, &OLSR_processMessage, &OLSR_createAttrs, NULL, &OLSR_destructor);
}

static list* compute_broadcast_mprs(NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time) {
    return compute_mprs(true, neighbors, myID, current_time);
}

static list* compute_routing_mprs(NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time) {
    return compute_mprs(false, neighbors, myID, current_time);
}

static list* compute_mprs(bool broadcast, NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time) {
    hash_table* n1 = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    list* n2 = list_init();

    void* iterator = NULL;
    NeighborEntry* current_neigh = NULL;
    while ( (current_neigh = NT_nextNeighbor(neighbors, &iterator)) ) {
        if( NE_getNeighborType(current_neigh, current_time) == BI_NEIGH /* && willingness > NEVER*/ ) {

            // Iterate through the 2 hop neighbors
            list* ns = list_init();

            hash_table* nneighs = NE_getTwoHopNeighbors(current_neigh);
            void* iterator2 = NULL;
            hash_table_item* hit = NULL;
            TwoHopNeighborEntry* current_2hop_neigh = NULL;
            while ( (hit = hash_table_iterator_next(nneighs, &iterator2)) ) {
                current_2hop_neigh = (TwoHopNeighborEntry*)hit->value;

                bool to_add = THNE_isBi(current_2hop_neigh) && uuid_compare(THNE_getID(current_2hop_neigh), myID) != 0 && list_find_item(n2, &equalN2Tuple, THNE_getID(current_2hop_neigh)) == NULL;

                if( to_add ) {
                    N2_Tuple* n2_tuple = newN2Tuple(THNE_getID(current_2hop_neigh), THNE_getTxLinkQuality(current_2hop_neigh)); // lq from 1 hop to 2 hops
                    list_add_item_to_tail(ns, n2_tuple);

                    unsigned char* id = malloc(sizeof(uuid_t));
                    uuid_copy(id, THNE_getID(current_2hop_neigh));
                    list_add_item_to_tail(n2, id);
                }
            }

            double lq = broadcast ? NE_getTxLinkQuality(current_neigh) : NE_getRxLinkQuality(current_neigh);
            N1_Tuple* n1_tuple = newN1Tuple(NE_getNeighborID(current_neigh), lq, DEFAULT_WILLINGNESS, ns);
            void* old = hash_table_insert(n1, n1_tuple->id, n1_tuple);
            assert(old == NULL);
        }
    }

    list* mprs = compute_multipoint_relays(n1, n2, NULL);

    hash_table_delete_custom(n1, &delete_n1_item, NULL);
    list_delete(n2);

    return mprs;
}
