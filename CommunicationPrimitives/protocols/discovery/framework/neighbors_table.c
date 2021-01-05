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

#include "neighbors_table.h"

#include "data_structures/hash_table.h"
#include "data_structures/graph.h"

#include "utility/my_misc.h"
#include "utility/my_string.h"
#include "utility/my_time.h"
#include "utility/my_math.h"
//#include "utility/byte.h"

#include <assert.h>

typedef struct NeighborsTable_ {
    hash_table* neighbors;
} NeighborsTable;

typedef struct NeighborEntry_ {
    // Ids
    uuid_t id;
    WLANAddr mac_addr;
    unsigned short seq;
    unsigned short hseq;

    // Periods
    unsigned long hello_period_s;
    unsigned long hack_period_s;

    // Timestamps
    struct timespec last_neighbor_timer;
    struct timespec found_time;
    struct timespec lost_time;
    // bool deleted;

    struct timespec rx_exp_time;
    struct timespec tx_exp_time;
    struct timespec removal_time;

    // Link Quality
    double tx_lq;
    double rx_lq;
    void* lq_attributes;

    // Link Admission
    bool pending;
    bool accepted;

    // Traffic
    double out_traffic;

    // Neighbors
    hash_table* neighs;

    // Extra Attributes
    void* context_attributes;

} NeighborEntry;

typedef struct TwoHopNeighborEntry_ {
    uuid_t id;
    unsigned short hseq;
    bool is_bi;
    double rx_lq;
    double tx_lq;
    double traffic;
    struct timespec expiration;
} TwoHopNeighborEntry;

NeighborsTable* newNeighborsTable(/*unsigned int n_bucket, unsigned int bucket_duration_s*/) {
    NeighborsTable* nt = malloc(sizeof(NeighborsTable));

    nt->neighbors = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    return nt;
}

static void deleteNeighborEntry_custom(hash_table_item* it, void* args) {
    void* lq_attributes = NULL, * msg_attributes = NULL;

    destroyNeighborEntry((NeighborEntry*)(it->value), &lq_attributes, &msg_attributes);

    if(lq_attributes) {
        void (*lq_destroy)(void*, void*) = ((void**)args)[0];
        void* lqm = ((void**)args)[3];
        lq_destroy(lqm, lq_attributes);
    }

    if(msg_attributes) {
        void (*msg_destroy)(void*, void*) = ((void**)args)[1];
        void* d_message = ((void**)args)[4];
        msg_destroy(d_message, msg_attributes);
    }
}

void destroyNeighborsTable(NeighborsTable* nt, void (*lq_destroy)(void*, void*), void (*msg_destroy)(void*, void*), void* lqm, void* d_message) {
    if(nt) {
        hash_table_delete_custom(nt->neighbors, &deleteNeighborEntry_custom, (void*[]){lq_destroy, msg_destroy, lqm, d_message});
        free(nt);
    }
}

unsigned int NT_getSize(NeighborsTable* nt) {
    return nt->neighbors->n_items;
}

void NT_addNeighbor(NeighborsTable* nt, NeighborEntry* neigh) {
    assert(nt && neigh);
    void* old = hash_table_insert(nt->neighbors, neigh->id, neigh);
    assert(old == NULL);
}

NeighborEntry* NT_getNeighbor(NeighborsTable* nt, unsigned char* neigh_id) {
    assert(nt && neigh_id);
    return (NeighborEntry*)hash_table_find_value(nt->neighbors, neigh_id);
}

NeighborEntry* NT_getNeighborByAddr(NeighborsTable* nt, WLANAddr* neigh_addr) {
    assert(nt && neigh_addr);

    void* iterator = NULL;
    NeighborEntry* current_neigh = NULL;
    while ( (current_neigh = NT_nextNeighbor(nt, &iterator)) ) {
        if( memcmp(NE_getNeighborMAC(current_neigh)->data, neigh_addr->data, WLAN_ADDR_LEN) == 0 ) {
            break;
        }
    }

    if( iterator ) {
        free(iterator);
    }

    return current_neigh;
}

NeighborEntry* NT_removeNeighbor(NeighborsTable* nt, unsigned char* neigh_id) {
    assert(nt);

    hash_table_item* it = hash_table_remove_item(nt->neighbors, neigh_id);
    if(it) {
        NeighborEntry* entry = (NeighborEntry*)it->value;
        free(it);
        return entry;
    } else {
        return NULL;
    }
}

NeighborEntry* newNeighborEntry(WLANAddr* mac_addr, unsigned char* id, unsigned short seq, unsigned long hello_period_s, double out_traffic, struct timespec* rx_exp_time, struct timespec* found_time) {

    NeighborEntry* neigh = malloc(sizeof(NeighborEntry));

    memcpy(neigh->mac_addr.data, mac_addr->data, WLAN_ADDR_LEN);
    uuid_copy(neigh->id, id);
    neigh->seq = seq;
    neigh->hseq = 0;

    neigh->hello_period_s = hello_period_s;
    neigh->hack_period_s = 0;

    // Timestamps
    copy_timespec(&neigh->last_neighbor_timer, found_time);
    copy_timespec(&neigh->found_time, found_time);
    copy_timespec(&neigh->lost_time, &zero_timespec);
    copy_timespec(&neigh->rx_exp_time, rx_exp_time);
    copy_timespec(&neigh->tx_exp_time, &zero_timespec);
    copy_timespec(&neigh->removal_time, &zero_timespec);

    // Link Quality
    neigh->tx_lq = 0.0;
    neigh->rx_lq = 0.0;
    neigh->lq_attributes = NULL;

    // Link Admission
    neigh->pending = true;
    neigh->accepted = false;

    // Traffic
    neigh->out_traffic = out_traffic;

    // Neighbors
    neigh->neighs = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    // Extra Attributes
    neigh->context_attributes = NULL;

    return neigh;
}

static void deleteTwoHopNeighborEntry_custom(hash_table_item* it, void* args) {
    free((TwoHopNeighborEntry*)(it->value));
}

void destroyNeighborEntry(NeighborEntry* neigh, void** lq_attributes, void** context_attributes) {

    if(neigh) {
        *lq_attributes = neigh->lq_attributes;
        *context_attributes = neigh->context_attributes;

        hash_table_delete_custom(neigh->neighs, &deleteTwoHopNeighborEntry_custom, NULL);

        free(neigh);
    }
}

unsigned char* NE_getNeighborID(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->id;
}

WLANAddr* NE_getNeighborMAC(NeighborEntry* neigh) {
    assert(neigh);
    return &neigh->mac_addr;
}

unsigned short NE_getNeighborSEQ(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->seq;
}

void NE_setNeighborSEQ(NeighborEntry* neigh, unsigned short seq) {
    assert(neigh);
    neigh->seq = seq;
}

unsigned short NE_getNeighborHSEQ(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->hseq;
}

void NE_setNeighborHSEQ(NeighborEntry* neigh, unsigned short hseq) {
    assert(neigh);
    neigh->hseq = hseq;
}

struct timespec* NE_getNeighborFoundTime(NeighborEntry* neigh) {
    assert(neigh);
    return &neigh->found_time;
}

struct timespec* NE_getNeighborLostTime(NeighborEntry* neigh) {
    assert(neigh);
    return &neigh->lost_time;
}

bool NE_isLost(NeighborEntry* neigh) {
    assert(neigh);

    bool lost = compare_timespec(&neigh->lost_time, (struct timespec*)&zero_timespec) != 0;

    //assert( !lost || (compare_timespec(&neigh->rx_exp_time, &neigh->lost_time) <= 0 /*&& compare_timespec(&neigh->tx_exp_time, &neigh->deleted_time) <= 0*/ ) );

    assert( !lost || ( !neigh->accepted || compare_timespec(&neigh->rx_exp_time, &neigh->lost_time) <= 0 ));

    /*if(deleted) {
        assert(neigh->accepted == false);
    }*/

    return lost;
}

void NE_setLost(NeighborEntry* neigh, struct timespec* current_time) {
    assert(neigh);

    assert(!neigh->accepted || compare_timespec(&neigh->rx_exp_time, current_time) <= 0);

    copy_timespec(&neigh->lost_time, current_time);
}

struct timespec* NE_getNeighborRxExpTime(NeighborEntry* neigh) {
    assert(neigh);
    return &neigh->rx_exp_time;
}

struct timespec* NE_getNeighborTxExpTime(NeighborEntry* neigh) {
    assert(neigh);
    return &neigh->tx_exp_time;
}

struct timespec* NE_getNeighborRemovalTime(NeighborEntry* neigh) {
    assert(neigh);
    return &neigh->removal_time;
}

DiscoveryNeighborType NE_getNeighborType(NeighborEntry* neigh, struct timespec* current_time) {
    assert(neigh);

    if(neigh->pending) {
        assert(!neigh->accepted);
        return PENDING_NEIGH;
    } else {
        // neigh timer could have not expired yet so neigh is not (notified as) lost yet
        if(compare_timespec(&neigh->rx_exp_time, current_time) >= 0 && !NE_isLost(neigh)) {
            if(compare_timespec(&neigh->tx_exp_time, current_time) >= 0) {
                return BI_NEIGH;
            } else {
                return UNI_NEIGH;
            }
        } else {
            return LOST_NEIGH;
        }
    }
}

unsigned long NE_getNeighborHelloPeriod(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->hello_period_s;
}

void NE_setNeighborHelloPeriod(NeighborEntry* neigh, unsigned long new_period_s) {
    assert(neigh);
    neigh->hello_period_s = new_period_s;
}

unsigned long NE_getNeighborHackPeriod(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->hack_period_s;
}

void NE_setNeighborHackPeriod(NeighborEntry* neigh, unsigned long new_period_s) {
    assert(neigh);
    neigh->hack_period_s = new_period_s;
}

struct timespec* NE_getLastNeighborTimer(NeighborEntry* neigh) {
    assert(neigh);
    return &neigh->last_neighbor_timer;
}

void NE_setLastNeighborTimer(NeighborEntry* neigh, struct timespec* current_time) {
    assert(neigh);
    copy_timespec(&neigh->last_neighbor_timer, current_time);
}

void NE_setNeighborRxExpTime(NeighborEntry* neigh, struct timespec* rx_exp_time) {
    assert(neigh);
    copy_timespec(&neigh->rx_exp_time, rx_exp_time);
}

void NE_setNeighborTxExpTime(NeighborEntry* neigh, struct timespec* tx_exp_time) {
    assert(neigh);
    copy_timespec(&neigh->tx_exp_time, tx_exp_time);
}

void NE_setNeighborRemovalTime(NeighborEntry* neigh, struct timespec* removal_time) {
    assert(neigh);
    copy_timespec(&neigh->removal_time, removal_time);
}

double NE_getRxLinkQuality(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->rx_lq;
}

double NE_getTxLinkQuality(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->tx_lq;
}

void NE_setRxLinkQuality(NeighborEntry* neigh, double rx_lq) {
    assert(neigh);
    neigh->rx_lq = rx_lq;
}

void NE_setTxLinkQuality(NeighborEntry* neigh, double tx_lq) {
    assert(neigh);
    neigh->tx_lq = tx_lq;
}

void* NE_setLinkQualityAttributes(NeighborEntry* neigh, void* lq_attributes) {
    assert(neigh);

    void* old_attributes = neigh->lq_attributes;
    neigh->lq_attributes = lq_attributes;
    return old_attributes;
}

void* NE_getLinkQualityAttributes(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->lq_attributes;
}

bool NE_isPending(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->pending;
}

void NE_setPending(NeighborEntry* neigh, bool pending) {
    assert(neigh);
    neigh->pending = pending;
}

bool NE_isAccepted(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->accepted;
}

void NE_setAccepted(NeighborEntry* neigh, bool accepted) {
    assert(neigh);
    neigh->accepted = accepted;
}

double NE_getOutTraffic(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->out_traffic;
}

void NE_setOutTraffic(NeighborEntry* neigh, double traffic) {
    assert(neigh);
    neigh->out_traffic = traffic;
}

void* NE_getContextAttributes(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->context_attributes;
}

void* NE_setContextAttributes(NeighborEntry* neigh, void* context_attributes) {
    assert(neigh);
    void* old_attributes = neigh->context_attributes;
    neigh->context_attributes = context_attributes;
    return old_attributes;
}

NeighborEntry* NT_nextNeighbor(NeighborsTable* nt, void** iterator) {
    hash_table_item* item = hash_table_iterator_next(nt->neighbors, iterator);
    if(item) {
        return (NeighborEntry*)(item->value);
    } else {
        return NULL;
    }
}

TwoHopNeighborEntry* newTwoHopNeighborEntry(unsigned char* id, unsigned short seq, bool is_bi, double rx_lq, double tx_lq, double traffic, struct timespec* expiration) {
    TwoHopNeighborEntry* nt = malloc(sizeof(TwoHopNeighborEntry));

    uuid_copy(nt->id, id);
    nt->hseq = seq;
    nt->is_bi = is_bi;
    nt->rx_lq = rx_lq;
    nt->tx_lq = tx_lq;
    nt->traffic = traffic;
    copy_timespec(&nt->expiration, expiration);

    return nt;
}

unsigned char* THNE_getID(TwoHopNeighborEntry* two_hop_neigh) {
    assert(two_hop_neigh);

    return two_hop_neigh->id;
}

unsigned short THNE_getHSEQ(TwoHopNeighborEntry* two_hop_neigh) {
    assert(two_hop_neigh);

    return two_hop_neigh->hseq;
}

void THNE_setHSEQ(TwoHopNeighborEntry* two_hop_neigh, unsigned short new_hseq) {
    assert(two_hop_neigh);

    two_hop_neigh->hseq = new_hseq;
}

bool THNE_isBi(TwoHopNeighborEntry* two_hop_neigh) {
    assert(two_hop_neigh);

    return two_hop_neigh->is_bi;
}

void THNE_setBi(TwoHopNeighborEntry* two_hop_neigh, bool is_bi) {
    assert(two_hop_neigh);

    two_hop_neigh->is_bi = is_bi;
}

double THNE_getRxLinkQuality(TwoHopNeighborEntry* two_hop_neigh) {
    assert(two_hop_neigh);

    return two_hop_neigh->rx_lq;
}

void THNE_setRxLinkQuality(TwoHopNeighborEntry* two_hop_neigh, double new_rx_lq) {
    assert(two_hop_neigh);

    two_hop_neigh->rx_lq = new_rx_lq;
}

void THNE_setTxLinkQuality(TwoHopNeighborEntry* two_hop_neigh, double new_tx_lq) {
    assert(two_hop_neigh);

    two_hop_neigh->tx_lq = new_tx_lq;
}

double THNE_getTxLinkQuality(TwoHopNeighborEntry* two_hop_neigh) {
    assert(two_hop_neigh);

    return two_hop_neigh->tx_lq;
}

double THNE_getTraffic(TwoHopNeighborEntry* two_hop_neigh) {
    assert(two_hop_neigh);

    return two_hop_neigh->traffic;
}

void THNE_setTraffic(TwoHopNeighborEntry* two_hop_neigh, double new_traffic) {
    assert(two_hop_neigh);

    two_hop_neigh->traffic = new_traffic;
}

struct timespec* THNE_getExpiration(TwoHopNeighborEntry* two_hop_neigh) {
    assert(two_hop_neigh);

    return &two_hop_neigh->expiration;
}

void THNE_setExpiration(TwoHopNeighborEntry* two_hop_neigh, struct timespec* new_expiration) {
    assert(two_hop_neigh);

    copy_timespec(&two_hop_neigh->expiration, new_expiration);
}

hash_table* NE_getTwoHopNeighbors(NeighborEntry* neigh) {
    assert(neigh);

    return neigh->neighs;
}

TwoHopNeighborEntry* NE_getTwoHopNeighborEntry(NeighborEntry* neigh, unsigned char* nn_id) {
    assert(neigh);

    TwoHopNeighborEntry* nn = hash_table_find_value(neigh->neighs, nn_id);

    return nn;
}

TwoHopNeighborEntry* NE_removeTwoHopNeighborEntry(NeighborEntry* neigh, unsigned char* nn_id) {
    assert(neigh);

    hash_table_item* hit = hash_table_remove_item(neigh->neighs, nn_id);
    if(hit) {
        TwoHopNeighborEntry* aux = hit->value;
        free(hit);
        return aux;
    } else {
        return NULL;
    }
}

TwoHopNeighborEntry* NE_addTwoHopNeighborEntry(NeighborEntry* neigh, TwoHopNeighborEntry* nn) {
    assert(neigh && nn);

    TwoHopNeighborEntry* aux = NE_removeTwoHopNeighborEntry(neigh, nn->id);

    hash_table_insert(neigh->neighs, nn->id, nn);

    return aux;
}

char* NT_print(NeighborsTable* nt, char** str, struct timespec* current_time, unsigned char* myID, WLANAddr* myMAC, unsigned short my_seq) {
    char* header = " # |                 ID                   |"
    "        MAC        |  SEQ  | T | A |   PERIODS   |"
    " RX_LQ | TX_LQ | TRAFFIC | RX_EXP | TX_EXP | FOUND  |  LOST  | REMOVE | NEIGHS \n";

    unsigned int line_size = strlen(header) + 1;

    unsigned int nneighs = 0;
    void* iterator = NULL;
    for(NeighborEntry* current_neigh = NT_nextNeighbor(nt, &iterator); current_neigh; current_neigh = NT_nextNeighbor(nt, &iterator)) {
        nneighs = lMax(nneighs, NE_getTwoHopNeighbors(current_neigh)->n_items);
    }

    unsigned long buffer_size = NT_getSize(nt)*(line_size*(nneighs+1)) + 3*line_size;

    char* buffer = malloc(buffer_size);
    char* ptr = buffer;

    // Print Column Headers
    sprintf(ptr, "%s", header);
    ptr += strlen(ptr);

    // Print each neighbor
    unsigned int counter = 0;
    iterator = NULL;
    NeighborEntry* current_neigh = NULL;
    while( (current_neigh = NT_nextNeighbor(nt, &iterator)) ) {

        char id_str[UUID_STR_LEN+1];
        uuid_unparse(NE_getNeighborID(current_neigh), id_str);
        id_str[UUID_STR_LEN] = '\0';

        char addr_str[20];
        wlan2asc(NE_getNeighborMAC(current_neigh), addr_str);
        align_str(addr_str, addr_str, 17, "CL");

        char seq_str[6];
        sprintf(seq_str, "%hu", NE_getNeighborSEQ(current_neigh));
        align_str(seq_str, seq_str, 5, "R");

        char* type_str = "";
        switch(NE_getNeighborType(current_neigh, current_time)) {
            case PENDING_NEIGH: type_str = "P"; break;
            case UNI_NEIGH: type_str = "U"; break;
            case BI_NEIGH: type_str = "B"; break;
            case LOST_NEIGH:
            default: type_str = "L";
        }

        struct timespec aux_t;

        char remove_str[7];
        char lost_str[7];
        if(NE_isLost(current_neigh)) {
            subtract_timespec(&aux_t, NE_getNeighborRemovalTime(current_neigh), current_time);
            timespec_to_string(&aux_t, remove_str, 6, 1);
            align_str(remove_str, remove_str, 6, "CR");

            subtract_timespec(&aux_t, current_time, NE_getNeighborLostTime(current_neigh));
            timespec_to_string(&aux_t, lost_str, 6, 1);
            align_str(lost_str, lost_str, 6, "CR");
        } else {
            sprintf(remove_str, "   -  ");
            sprintf(lost_str, "   -  ");
        }

        char rx_exp_str[7];
        char rx_lq_str[6];
        if(compare_timespec(NE_getNeighborRxExpTime(current_neigh), current_time) <= 0 ) {
            sprintf(rx_lq_str, "  -  ");
            sprintf(rx_exp_str, "   -  ");
        } else {
            sprintf(rx_lq_str, "%0.3f", NE_getRxLinkQuality(current_neigh));

            subtract_timespec(&aux_t, NE_getNeighborRxExpTime(current_neigh), current_time);
            timespec_to_string(&aux_t, rx_exp_str, 6, 1);
            align_str(rx_exp_str, rx_exp_str, 6, "CR");
        }

        char tx_exp_str[7];
        char tx_lq_str[6];
        if(compare_timespec(NE_getNeighborTxExpTime(current_neigh), current_time) <= 0) {
            sprintf(tx_exp_str, "   -  ");
            sprintf(tx_lq_str, "  -  ");
        } else {
            sprintf(tx_lq_str, "%0.3f", NE_getTxLinkQuality(current_neigh));

            subtract_timespec(&aux_t, NE_getNeighborTxExpTime(current_neigh), current_time);
            timespec_to_string(&aux_t, tx_exp_str, 6, 1);
            align_str(tx_exp_str, tx_exp_str, 6, "CR");
        }

        char traffic_str[6];
        sprintf(traffic_str, "%0.3f" , NE_getOutTraffic(current_neigh));

        char found_str[7];
        subtract_timespec(&aux_t, current_time, NE_getNeighborFoundTime(current_neigh));
        timespec_to_string(&aux_t, found_str, 6, 1);
        align_str(found_str, found_str, 6, "CR");

        char neighs_str[7];
        sprintf(neighs_str, "%u", NE_getTwoHopNeighbors(current_neigh)->n_items);
        align_str(neighs_str, neighs_str, 6, "CR");

        char periods_str[12];
        sprintf(periods_str, "%d s %d s", (char)NE_getNeighborHelloPeriod(current_neigh), (char)NE_getNeighborHackPeriod(current_neigh));
        align_str(periods_str, periods_str, 11, "CR");

        char* accepted_str = NE_isAccepted(current_neigh) ? "T" : "F";

        sprintf(ptr, "%2.d   %s   %s   %s   %s   %s   %s   %s   %s   "
        " %s    %s   %s   %s   %s   %s   %s \n",
        counter+1, id_str, addr_str, seq_str, type_str, accepted_str, periods_str, rx_lq_str, tx_lq_str, traffic_str, rx_exp_str, tx_exp_str, found_str, lost_str, remove_str, neighs_str);
        ptr += strlen(ptr);

        // 2-hop neighs
        hash_table* neighs = NE_getTwoHopNeighbors(current_neigh);
        void* iterator = NULL;
        hash_table_item* hit = NULL;
        while( (hit = hash_table_iterator_next(neighs, &iterator)) ) {
            TwoHopNeighborEntry* nneigh = (TwoHopNeighborEntry*)hit->value;

            uuid_unparse(nneigh->id, id_str);
            id_str[UUID_STR_LEN] = '\0';

            sprintf(seq_str, "%hu", nneigh->hseq);
            align_str(seq_str, seq_str, 5, "R");

            type_str = nneigh->is_bi ? "B" : "U";

            sprintf(rx_lq_str, "%0.3f", nneigh->rx_lq);
            if( !nneigh->is_bi ) {
                sprintf(tx_lq_str, "  -  ");
            } else {
                sprintf(tx_lq_str, "%0.3f", nneigh->tx_lq);
            }

            subtract_timespec(&aux_t, &nneigh->expiration, current_time);
            timespec_to_string(&aux_t, rx_exp_str, 6, 1);
            align_str(rx_exp_str, rx_exp_str, 6, "CR");

            sprintf(traffic_str, "%0.3f" , nneigh->traffic);

            char c = uuid_compare(myID, nneigh->id) == 0 ? '*' : ' ';

            sprintf(ptr, "     %c   %s                   %s   %s                     %s   %s    %s             %s  \n",
            c, id_str, seq_str, type_str, rx_lq_str, tx_lq_str, traffic_str, rx_exp_str);
            ptr += strlen(ptr);
        }

        counter++;
    }

    assert(ptr <= buffer+buffer_size);

    *str = buffer;
    return *str;
}

void NT_serialize(NeighborsTable* nt, unsigned char* myID, WLANAddr* myMAC, double out_traffic, struct timespec* current_time, byte** buffer, unsigned int* size) {
    assert(nt);

    unsigned int value_size = WLAN_ADDR_LEN + sizeof(double);
    unsigned int label_size = sizeof(double);

    graph* g = graph_init_complete((key_comparator)&uuid_compare, NULL, NULL, sizeof(uuid_t), value_size, label_size);

    // Add myself
    {
        unsigned char* key = malloc(sizeof(uuid_t));
        uuid_copy(key, myID);

        byte* value = malloc(value_size);
        memcpy(value, myMAC->data, WLAN_ADDR_LEN);
        memcpy(value+WLAN_ADDR_LEN, &out_traffic, sizeof(double));

        graph_insert_node(g, key, value);
    }

    // Serialize each neighbor
    void* iterator = NULL;
    NeighborEntry* current_neigh = NULL;
    while( (current_neigh = NT_nextNeighbor(nt, &iterator)) ) {

        bool not_pending = !NE_isPending(current_neigh);
        if( !NE_isLost(current_neigh) && not_pending ) {

            unsigned char* neigh_id = NE_getNeighborID(current_neigh);

            graph_node* node = graph_find_node(g, neigh_id);
            if( node == NULL ) {
                unsigned char* key = malloc(sizeof(uuid_t));
                uuid_copy(key, neigh_id);

                byte* value = malloc(value_size);
                memcpy(value, NE_getNeighborMAC(current_neigh)->data, WLAN_ADDR_LEN);
                double traffic = NE_getOutTraffic(current_neigh);
                memcpy(value+WLAN_ADDR_LEN, &traffic, sizeof(double));

                graph_insert_node(g, key, value);
            }

            graph_edge* edge = graph_find_edge(g, neigh_id, myID);
            assert(edge == NULL);

            double* label = malloc(sizeof(double));
            *label = NE_getRxLinkQuality(current_neigh);

            graph_insert_edge(g, neigh_id, myID, label);

            if( NE_getNeighborType(current_neigh, current_time) == BI_NEIGH ) {
                edge = graph_find_edge(g, myID, neigh_id);
                assert(edge == NULL);
                //if(edge == NULL) {
                    double* label = malloc(sizeof(double));
                    *label = NE_getTxLinkQuality(current_neigh);

                    graph_insert_edge(g, myID, neigh_id, label);
                //}
            } else {
                label = graph_remove_edge(g, myID, neigh_id);
                if(label)
                    free(label);
            }

            hash_table* neighs = NE_getTwoHopNeighbors(current_neigh);
            void* iterator = NULL;
            hash_table_item* hit = NULL;
            while( (hit = hash_table_iterator_next(neighs, &iterator)) ) {
                TwoHopNeighborEntry* current_neigh_2 = (TwoHopNeighborEntry*)hit->value;

                unsigned char* neigh2_id = THNE_getID(current_neigh_2);

                if( uuid_compare(neigh2_id, myID) != 0 ) {
                    graph_node* node = graph_find_node(g, neigh2_id);
                    if( node == NULL ) {
                        unsigned char* key = malloc(sizeof(uuid_t));
                        uuid_copy(key, neigh2_id);

                        byte* value = malloc(value_size);
                        memset(value, 0, WLAN_ADDR_LEN);
                        double traffic = THNE_getTraffic(current_neigh_2);
                        memcpy(value+WLAN_ADDR_LEN, &traffic, sizeof(double));

                        graph_insert_node(g, key, value);
                    }

                    edge = graph_find_edge(g, neigh2_id, neigh_id);
                    assert(edge == NULL);

                    label = malloc(sizeof(double));
                    *label = THNE_getRxLinkQuality(current_neigh_2);

                    graph_insert_edge(g, neigh2_id, neigh_id, label);

                    // Is not two hop neigh also and is bi
                    if( NT_getNeighbor(nt, neigh2_id) == NULL && THNE_isBi(current_neigh_2) ) {
                        edge = graph_find_edge(g, neigh_id, neigh2_id);
                        assert(edge == NULL);

                        label = malloc(sizeof(double));
                        *label = THNE_getTxLinkQuality(current_neigh_2);

                        graph_insert_edge(g, neigh_id, neigh2_id, label);
                    }
                }
            }
        } else {
            // Ignore entry
        }
    }

    unsigned int buffer_total_size = 2*sizeof(byte) + g->nodes->size*(sizeof(uuid_t) + WLAN_ADDR_LEN + sizeof(double)) + g->edges->size*(2*sizeof(uuid_t) + sizeof(double));
    byte* buffer_ = malloc(buffer_total_size);
    byte* ptr = buffer_;

    // Serialize nodes
    byte n_nodes = g->nodes->size;
    memcpy(ptr, &n_nodes, sizeof(n_nodes));
    ptr += sizeof(byte);

    for(list_item* it = g->nodes->head; it; it = it->next) {
        graph_node* node = (graph_node*)it->data;

        // Serialize node id
        memcpy(ptr, node->key, sizeof(uuid_t));
        ptr += sizeof(uuid_t);

        // Serialize MAC + out traffic
        memcpy(ptr, node->value, value_size);
        ptr += value_size;
    }

    // Serialize edges
    byte n_edges = g->edges->size;
    memcpy(ptr, &n_edges, sizeof(n_edges));
    ptr += sizeof(byte);

    for(list_item* it = g->edges->head; it; it = it->next) {
        graph_edge* edge = (graph_edge*)it->data;

        // Serialize start node id
        memcpy(ptr, edge->start_node->key, sizeof(uuid_t));
        ptr += sizeof(uuid_t);

        // Serialize end node id
        memcpy(ptr, edge->end_node->key, sizeof(uuid_t));
        ptr += sizeof(uuid_t);

        // Serialize link quality
        memcpy(ptr, edge->label, label_size);
        ptr += label_size;
    }

    graph_delete(g);

    *buffer = buffer_;
    *size = buffer_total_size;
}
