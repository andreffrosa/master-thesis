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

#include "discovery_context_private.h"

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

static list* compute_mprs(bool broadcast, NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time, list* old_mprs);
static list* compute_broadcast_mprs(NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time, list* old_mprs);
static list* compute_routing_mprs(NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time, list* old_mprs);
static NeighMPRType getMPRType(bool flooding_mpr, bool routing_mpr);
static bool recompute_mprs(OLSRState* state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors);
static void notify(void* f_state, char* type, list* flooding_set, list* routing_set);

static bool update(void* f_state, OLSRState* state, unsigned char* myID, NeighborsTable* neighbors, struct timespec* current_time, bool one_hop_change, bool two_hop_change);

static void OLSR_createMessage(ModuleState* m_state, unsigned char* myID, NeighborsTable* neighbors, DiscoveryInternalEventType event_type, void* event_args, struct timespec* current_time, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size) {
    assert(hello);

    //OLSRState* state = (OLSRState*)m_state->vars;

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
        OLSRAttrs* neigh_attrs = NE_getContextAttributes(neigh);

        NeighMPRType mpr_type = getMPRType(neigh_attrs->flooding_mpr, neigh_attrs->routing_mpr);
        byte aux = mpr_type;
        memcpy(ptr, &aux, sizeof(aux));
        ptr += sizeof(aux);
        *size += sizeof(aux);
    }

}

static bool OLSR_processMessage(ModuleState* m_state, void* f_state, unsigned char* myID, NeighborsTable* neighbors, struct timespec* current_time, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size, MessageSummary* msg_summary) {

    OLSRState* state = (OLSRState*)m_state->vars;

    bool one_hop_change = false;
    bool two_hop_change = false;

    byte* ptr = buffer;

    // Deserialize Hello
    HelloMessage hello;
    memcpy(&hello, ptr, sizeof(HelloMessage));
    ptr += sizeof(HelloMessage);

    HelloDeliverSummary* summary = deliverHello(f_state, &hello, mac_addr, msg_summary);
    one_hop_change |= summary->new_neighbor || summary->updated_neighbor;
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

            HackDeliverSummary* summary = deliverHack(f_state, &hacks[i], msg_summary);
            one_hop_change |= summary->updated_neighbor;
            two_hop_change |= summary->updated_2hop_neighbor || summary->added_2hop_neighbor || summary->lost_2hop_neighbor;
            free(summary);

            byte aux;
            memcpy(&aux, ptr, sizeof(aux));
            ptr += sizeof(aux);
            NeighMPRType mpr_type = aux;

            if( uuid_compare(hacks[i].dest_process_id, myID) == 0 ) {
                NeighborEntry* neigh = NT_getNeighbor(neighbors, hacks[i].src_process_id);
                assert(neigh);
                OLSRAttrs* neigh_attrs = NE_getContextAttributes(neigh);

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
        notify(f_state, "MPR SELECTORS", state->flooding_mpr_selectors, state->routing_mpr_selectors);
    }

    bool changed_mprs = update(f_state, state, myID, neighbors, current_time, one_hop_change, two_hop_change);

    return changed_mpr_selectors || changed_mprs;
}

static bool OLSRDiscovery_updateContext(ModuleState* m_state, void* f_state, unsigned char* myID, NeighborEntry* neighbor, NeighborsTable* neighbors, struct timespec* current_time, NeighborTimerSummary* summary) {
    OLSRState* state = (OLSRState*)m_state->vars;

    // Recompute mprs if necessary
    bool one_hop_change = /*summary->new_neighbor ||*/ summary->updated_neighbor || summary->lost_neighbor;
    bool two_hop_change = /*summary->updated_2hop_neighbor || summary->added_2hop_neighbor ||*/ summary->lost_2hop_neighbor;

    bool aux1 = update_mprs(f_state, state, myID, neighbors, current_time, one_hop_change, two_hop_change);

    bool aux2 = update_mpr_selectors(f_state, state, myID, neighbor, current_time, neighbors, summary->lost_neighbor);

    return aux1 || aux2;
}


static void OLSR_destructor(ModuleState* m_state) {
    OLSRState* state = (OLSRState*)m_state->vars;

    list_delete(state->flooding_mprs);
    list_delete(state->flooding_mpr_selectors);
    list_delete(state->routing_mprs);
    list_delete(state->routing_mpr_selectors);

    free(state);
}

DiscoveryContext* OLSRDiscoveryContext() {
    OLSRState* state = malloc(sizeof(OLSRState));
    state->flooding_mprs = list_init();
    state->flooding_mpr_selectors = list_init();
    state->routing_mprs = list_init();
    state->routing_mpr_selectors = list_init();

    return newDiscoveryContext(NULL, state, &OLSR_createMessage, &OLSR_processMessage, &OLSRDiscovery_updateContext, &OLSR_createAttrs, NULL, &OLSR_destructor);
}

static bool update_mpr_selectors(void* f_state, OLSRState* state, unsigned char* myID, NeighborEntry* neighbor, struct timespec* current_time, NeighborsTable* neighbors, bool lost_neighbor) {

    OLSRAttrs* neigh_attrs = NE_getContextAttributes(neigh);

    if(lost_neighbor) {
        bool changed = false;

        if( list_find_item(state->flooding_mpr_selectors, &equalID, NE_getNeighborID(neigh)) != NULL ) {
            neigh_attrs->flooding_mpr_selector = false;

            void* aux = list_remove_item(state->flooding_mpr_selectors, &equalID, NE_getNeighborID(neigh));
            if(aux) {
                free(aux);
            }

            changed = true;
        }

        if( list_find_item(state->routing_mpr_selectors, &equalID, NE_getNeighborID(neigh)) != NULL ) {
            neigh_attrs->routing_mpr_selector = false;

            void* aux = list_remove_item(state->routing_mpr_selectors, &equalID, NE_getNeighborID(neigh));
            if(aux) {
                free(aux);
            }

            changed = true;
        }

        // Notify
        if( changed ) {
            notify(f_state, "MPR SELECTORS", state->flooding_mprs, state->routing_mprs);
        }
    }

    return false;
}

static bool update_mprs(void* f_state, OLSRState* state, unsigned char* myID, NeighborsTable* neighbors, struct timespec* current_time, bool one_hop_change, bool two_hop_change) {

    if( one_hop_change || two_hop_change ) {
        bool changed = recompute_mprs(state, myID, current_time, neighbors);

        // Notify
        if( changed ) {
            notify(f_state, "MPRS", state->flooding_mprs, state->routing_mprs);
        }

        return changed;

        // if the mprs didn't change and the changes where only at two hop neighbors and is not periodic, then don't send a message
        /*
        if( !changed && !one_hop_change && two_hop_change ) {
        return false;
        }
        */
    }

    return false;
}

static void notify(void* f_state, char* type, list* flooding_set, list* routing_set) {

    unsigned int size = 2*sizeof(unsigned int) + (flooding_set->size + routing_set->size)*sizeof(uuid_t);
    byte buffer[size];
    byte* ptr = buffer;

    unsigned int aux = flooding_set->size;
    memcpy(ptr, &aux, sizeof(unsigned int));
    ptr += sizeof(unsigned int);

    for( list_item* it = flooding_set->head; it; it = it->next ) {
        unsigned char* id = (unsigned char*)it->data;

        memcpy(ptr, id, sizeof(uuid_t));
        ptr += sizeof(uuid_t);
    }

    aux = routing_set->size;
    memcpy(ptr, &aux, sizeof(unsigned int));
    ptr += sizeof(unsigned int);

    for( list_item* it = routing_set->head; it; it = it->next ) {
        unsigned char* id = (unsigned char*)it->data;

        memcpy(ptr, id, sizeof(uuid_t));
        ptr += sizeof(uuid_t);
    }

    DF_notifyGenericEvent(f_state, type, buffer, size);
}

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

    list* new_flooding_mprs = compute_broadcast_mprs(neighbors, myID, current_time, state->flooding_mprs);
    list* new_routing_mprs = compute_routing_mprs(neighbors, myID, current_time, state->routing_mprs);

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
        // Refresh Neighbor's Attributes
        void* iterator = NULL;
        NeighborEntry* current_neigh = NULL;
        OLSRAttrs* neigh_attrs = NULL;
        while( (current_neigh = NT_nextNeighbor(neighbors, &iterator)) ) {
            neigh_attrs = NE_getContextAttributes(current_neigh);

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

static list* compute_broadcast_mprs(NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time, list* old_mprs) {
    return compute_mprs(true, neighbors, myID, current_time, old_mprs);
}

static list* compute_routing_mprs(NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time, list* old_mprs) {
    return compute_mprs(false, neighbors, myID, current_time, old_mprs);
}

static list* compute_mprs(bool broadcast, NeighborsTable* neighbors, unsigned char* myID, struct timespec* current_time, list* old_mprs) {
    hash_table* n1 = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    list* n2 = list_init();

    //printf("COMPUTING N1 AND N2:\n");

    void* iterator = NULL;
    NeighborEntry* current_neigh = NULL;
    while ( (current_neigh = NT_nextNeighbor(neighbors, &iterator)) ) {

        if( NE_getNeighborType(current_neigh, current_time) == BI_NEIGH /* && willingness > NEVER*/ ) {

            /*
            char id_str[UUID_STR_LEN+1] = {0};
            uuid_unparse(NE_getNeighborID(current_neigh), id_str);
            printf("%s\n", id_str);
            */

            list* ns = list_init();

            // Iterate through the 2 hop neighbors
            hash_table* nneighs = NE_getTwoHopNeighbors(current_neigh);
            void* iterator2 = NULL;
            hash_table_item* hit = NULL;
            TwoHopNeighborEntry* current_2hop_neigh = NULL;
            while ( (hit = hash_table_iterator_next(nneighs, &iterator2)) ) {
                current_2hop_neigh = (TwoHopNeighborEntry*)hit->value;

                bool is_bi = THNE_isBi(current_2hop_neigh);
                bool is_not_me = uuid_compare(THNE_getID(current_2hop_neigh), myID) != 0;
                bool is_not_in_n2_yet = list_find_item(n2, &equalN2Tuple, THNE_getID(current_2hop_neigh)) == NULL;

                /*
                char id_str2[UUID_STR_LEN+1] = {0};
                uuid_unparse(THNE_getID(current_2hop_neigh), id_str2);
                printf("    %s is_bi=%s is_not_me=%s is_not_in_n2_yet=%s\n", id_str2, (is_bi?"T":"F"), (is_not_me?"T":"F"), (is_not_in_n2_yet?"T":"F"));
                */



                if( is_bi && is_not_me ) {
                    if( is_not_in_n2_yet ) {
                        unsigned char* id = malloc(sizeof(uuid_t));
                        uuid_copy(id, THNE_getID(current_2hop_neigh));
                        list_add_item_to_tail(n2, id);
                    }

                    N2_Tuple* n2_tuple = newN2Tuple(THNE_getID(current_2hop_neigh), THNE_getTxLinkQuality(current_2hop_neigh)); // lq from 1 hop to 2 hops
                    list_add_item_to_tail(ns, n2_tuple);
                }
            }

            double lq = broadcast ? NE_getTxLinkQuality(current_neigh) : NE_getRxLinkQuality(current_neigh);
            bool already_mpr = list_find_item(old_mprs, &equalID, NE_getNeighborID(current_neigh)) != NULL;

            N1_Tuple* n1_tuple = newN1Tuple(NE_getNeighborID(current_neigh), lq, DEFAULT_WILLINGNESS, ns, already_mpr);
            void* old = hash_table_insert(n1, n1_tuple->id, n1_tuple);
            assert(old == NULL);
        }
    }

    list* mprs = compute_multipoint_relays(n1, n2, NULL);

    hash_table_delete_custom(n1, &delete_n1_item, NULL);
    list_delete(n2);

    return mprs;
}
