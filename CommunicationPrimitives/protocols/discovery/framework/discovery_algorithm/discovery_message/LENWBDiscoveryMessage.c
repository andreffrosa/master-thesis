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
#include "data_structures/hash_table.h"

typedef struct NeighPair_ {
    unsigned short seq;
    byte n_neighs;
} NeighPair;

typedef struct LENWBDiscoveryState_ {
    hash_table* strict_two_hop_neighbors_neighbors;
} LENWBDiscoveryState;

static void notify(LENWBDiscoveryState* state);

static bool LENWBDiscovery_createMessage(ModuleState* m_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, MessageType msg_type, void* aux_info, HelloMessage* hello, HackMessage* hacks, byte n_hacks, byte* buffer, unsigned short* size) {
    assert(hello);

    LENWBDiscoveryState* state = (LENWBDiscoveryState*)m_state->vars;

    //
    if( msg_type == NEIGHBOR_CHANGE_MSG ) {
        NeighborChangeSummary* s = (NeighborChangeSummary*)aux_info;

        bool one_hop_change = s->new_neighbor || s->updated_neighbor || s->lost_neighbor;
        bool two_hop_change = s->updated_2hop_neighbor || s->added_2hop_neighbor || s->lost_2hop_neighbor;

        if( one_hop_change || two_hop_change ) {
            bool changed = false;

            void* iterator = NULL;
            hash_table_item* hit = NULL;
            while( (hit = hash_table_iterator_next(state->strict_two_hop_neighbors_neighbors, &iterator)) ) {
                unsigned char* id = (unsigned char*)hit->key;
                NeighPair* pair = (NeighPair*)hit->value;

                bool strict_bi_two_hop_neigh = false;

                NeighborEntry* neigh = NT_getNeighbor(neighbors, id);
                if( neigh == NULL ) {
                    void* iterator2 = NULL;
                    NeighborEntry* current_neigh = NULL;
                    while( (current_neigh = NT_nextNeighbor(neighbors, &iterator2)) ) {
                        TwoHopNeighborEntry* two_hop_neigh = NE_getTwoHopNeighborEntry(current_neigh, id);
                        if( two_hop_neigh != NULL ) {
                            if( THNE_isBi(two_hop_neigh) ) {
                                strict_bi_two_hop_neigh = true;
                            }
                        }
                    }
                }
                if( !strict_bi_two_hop_neigh ) {
                    // Remove
                    NeighPair* aux = hash_table_remove(state->strict_two_hop_neighbors_neighbors, id);
                    assert(aux == pair);

                    free(pair);
                    changed = true;
                }
            }

            if( changed ) {
                notify(state);
            }

            if( !changed && !one_hop_change && two_hop_change ) {
                return false;
            }
        }
    }

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
        byte n_neighs = 0;
        hash_table* ht = NE_getTwoHopNeighbors(neigh);
        void* iterator = NULL;
        hash_table_item* hit = NULL;
        while( (hit = hash_table_iterator_next(ht, &iterator)) ) {
            TwoHopNeighborEntry* two_hop_neigh = (TwoHopNeighborEntry*)hit->value;
            if( THNE_isBi(two_hop_neigh) ) {
                n_neighs++;
            }
        }
        memcpy(ptr, &n_neighs, sizeof(n_neighs));
        ptr += sizeof(n_neighs);
        *size += sizeof(n_neighs);
    }

    return true;
}

static bool LENWBDiscovery_processMessage(ModuleState* m_state, void* f_state, unsigned char* myID, struct timespec* current_time, NeighborsTable* neighbors, bool piggybacked, WLANAddr* mac_addr, byte* buffer, unsigned short size) {

    LENWBDiscoveryState* state = (LENWBDiscoveryState*)m_state->vars;

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

    bool changed = false;

    if( n_hacks > 0 ) {
        HackMessage hacks[n_hacks];

        for(int i = 0; i < n_hacks; i++) {
            memcpy(&hacks[i], ptr, sizeof(HackMessage));
            ptr += sizeof(HackMessage);

            HackDeliverSummary* summary = deliverHack(f_state, &hacks[i]);
            free(summary);

            byte n_neighs = 0;
            memcpy(&n_neighs, ptr, sizeof(n_neighs));
            ptr += sizeof(n_neighs);

            if( uuid_compare(myID, hacks[i].dest_process_id) != 0 ) {
                NeighborEntry* neigh = NT_getNeighbor(neighbors, hacks[i].src_process_id);
                if( neigh ) {
                    TwoHopNeighborEntry* two_hop_neigh = NE_getTwoHopNeighborEntry(neigh, hacks[i].dest_process_id);

                    NeighborEntry* neigh2 = NT_getNeighbor(neighbors, hacks[i].dest_process_id);

                    bool strict_bi_two_hop_neigh = (neigh2 == NULL && two_hop_neigh != NULL && THNE_isBi(two_hop_neigh));

                    NeighPair* pair = hash_table_find_value(state->strict_two_hop_neighbors_neighbors, hacks[i].dest_process_id);

                    if( pair ) {
                        if( hacks[i].seq > pair->seq ) {
                            if( strict_bi_two_hop_neigh ) {
                                if( pair->n_neighs != n_neighs ) {
                                    // Update
                                    pair->seq = hacks[i].seq;
                                    pair->n_neighs = n_neighs;
                                    changed = true;
                                }
                            } else {
                                // Remove
                                NeighPair* aux = hash_table_remove(state->strict_two_hop_neighbors_neighbors, hacks[i].dest_process_id);
                                assert(aux == pair);

                                free(pair);
                                changed = true;
                            }
                        }
                    } else {
                        if( strict_bi_two_hop_neigh ) {
                            // Insert
                            unsigned char* key = malloc(sizeof(uuid_t));
                            uuid_copy(key, hacks[i].dest_process_id);

                            pair = malloc(sizeof(NeighPair));
                            pair->seq = hacks[i].seq;
                            pair->n_neighs = n_neighs;
                            changed = true;

                            void* old = hash_table_insert(state->strict_two_hop_neighbors_neighbors, key, pair);
                            assert(old == NULL);
                        }
                    }
                }
            }
        }
    }

    // Notify
    if( changed ) {
        notify(state);
    }

    return false;
}

static void notify(LENWBDiscoveryState* state) {
    unsigned int size = sizeof(unsigned int) + state->strict_two_hop_neighbors_neighbors->n_items*(sizeof(uuid_t) + sizeof(byte));

    byte buffer[size];
    byte* ptr = buffer;

    unsigned int amount = state->strict_two_hop_neighbors_neighbors->n_items;
    memcpy(ptr, &amount, sizeof(amount));
    ptr += sizeof(amount);

    void* iterator = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(state->strict_two_hop_neighbors_neighbors, &iterator)) ) {
        NeighPair* pair = (NeighPair*)hit->value;

        memcpy(ptr, hit->key, sizeof(uuid_t));
        ptr += sizeof(uuid_t);

        memcpy(ptr, &pair->n_neighs, sizeof(byte));
        ptr += sizeof(byte);
    }

    DF_notifyGenericEvent("LENWB_NEIGHS", buffer, size);
}

static void LENWBDiscovery_destructor(ModuleState* m_state) {
    LENWBDiscoveryState* state = (LENWBDiscoveryState*)m_state->vars;

    hash_table_delete(state->strict_two_hop_neighbors_neighbors);

    free(state);
}

DiscoveryMessage* LENWBDiscoveryMessage() {
    LENWBDiscoveryState* state = malloc(sizeof(LENWBDiscoveryState));
    state->strict_two_hop_neighbors_neighbors = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    return newDiscoveryMessage(NULL,
        state,
        &LENWBDiscovery_createMessage,
        &LENWBDiscovery_processMessage,
        NULL,
        NULL,
        &LENWBDiscovery_destructor);
}
