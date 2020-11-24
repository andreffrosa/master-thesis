/*********************************************************
 * This code was written in the context of the Ligneighborskone
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
#include "utility/my_misc.h"
#include "utility/my_string.h"
#include "utility/my_time.h"
#include "utility/my_math.h"

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
    struct timespec deleted_time;
    // bool deleted;

    struct timespec rx_exp_time;
    struct timespec tx_exp_time;
    struct timespec removal_time;

    // Link Quality
    double tx_lq;
    double rx_lq;
    // double bi_lq;
    void* lq_attributes;

    // Traffic
    double out_traffic;
    //Window *misses;
    //Window* in_traffic;
    //Window* out_traffic;

    // YggMessage* last_announce;

    // Neighbors
    //list* neighs;
    hash_table* neighs;

    // Extra Attributes
    //unsigned int attributes_size;
    void* msg_attributes;

} NeighborEntry;

typedef struct TwoHopNeighborEntry_ {
    uuid_t id;
    unsigned short hseq;
    //DiscoveryNeighborType type; // TODO: or bool?
    bool is_bi;
    double rx_lq;
    double tx_lq;
    double traffic;
    struct timespec expiration;
} TwoHopNeighborEntry;

NeighborsTable* newNeighborsTable(/*unsigned int n_bucket, unsigned int bucket_duration_s*/) {
    NeighborsTable* nt = malloc(sizeof(NeighborsTable));

    nt->neighbors = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    /*
    nt->out_traffic = newWindow(n_bucket, bucket_duration_s);
    nt->instability = newWindow(n_bucket, bucket_duration_s);
    */

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

        /*
        destroyWindow(nt->out_traffic);
        destroyWindow(nt->instability);
        */

        free(nt);
    }
}

/*
Window* NT_getOutTraffic(NeighborsTable* nt) {
assert(nt);
return nt->out_traffic;
}

Window* NT_getInstability(NeighborsTable* nt) {
assert(nt);
return nt->instability;
}
*/

unsigned int NT_getSize(NeighborsTable* nt) {
    return nt->neighbors->n_items;
}

void NT_addNeighbor(NeighborsTable* nt, NeighborEntry* neigh) {
    assert(nt);
    void* old = hash_table_insert(nt->neighbors, neigh->id, neigh);
    assert(old == NULL);
}

NeighborEntry* NT_getNeighbor(NeighborsTable* nt, unsigned char* neigh_id) {
    assert(nt);
    return (NeighborEntry*)hash_table_find_value(nt->neighbors, neigh_id);
}

NeighborEntry* NT_removeNeighbor(NeighborsTable* nt, unsigned char* neigh_id) {
    assert(nt);

    hash_table_item* it = hash_table_remove(nt->neighbors, neigh_id);
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
    copy_timespec(&neigh->deleted_time, &zero_timespec);
    copy_timespec(&neigh->rx_exp_time, rx_exp_time);
    copy_timespec(&neigh->tx_exp_time, &zero_timespec);
    copy_timespec(&neigh->removal_time, &zero_timespec);

    // Link Quality
    neigh->tx_lq = 0.0;
    neigh->rx_lq = 0.0;
    //neigh->bi_lq = 0.0;
    neigh->lq_attributes = NULL;

    // Traffic
    neigh->out_traffic = out_traffic;

    // Neighbors
    neigh->neighs = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    // Extra Attributes
    //neigh->attributes_size = 0;
    neigh->msg_attributes = NULL;

    return neigh;
}

static void deleteTwoHopNeighborEntry_custom(hash_table_item* it, void* args) {
    free((TwoHopNeighborEntry*)(it->value));
}

void destroyNeighborEntry(NeighborEntry* neigh, void** lq_attributes, void** msg_attributes) {

    if(neigh) {
        *lq_attributes = neigh->lq_attributes;
        *msg_attributes = neigh->msg_attributes;

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

/*unsigned short NE_getNeighborVersion(NeighborEntry* neigh) {
assert(neigh);
return neigh->version;
}*/

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

/*
struct timespec* NE_getNeighborExpirationDate(NeighborEntry* neigh) {
assert(neigh);
return &neigh->expiration_date;
}
*/

struct timespec* NE_getNeighborFoundTime(NeighborEntry* neigh) {
    assert(neigh);
    return &neigh->found_time;
}

struct timespec* NE_getNeighborDeletedTime(NeighborEntry* neigh) {
    assert(neigh);
    return &neigh->deleted_time;
}

bool NE_isDeleted(NeighborEntry* neigh) {
    assert(neigh);

    bool deleted = compare_timespec(&neigh->deleted_time, (struct timespec*)&zero_timespec) != 0;

    assert( !deleted || \
        (compare_timespec(&neigh->rx_exp_time, &neigh->deleted_time) <= 0 /*&& compare_timespec(&neigh->tx_exp_time, &neigh->deleted_time) <= 0*/ ) );

        return deleted;
    }

    void NE_setDeleted(NeighborEntry* neigh, struct timespec* current_time) {
        assert(neigh);
        copy_timespec(&neigh->deleted_time, current_time);
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

        if(compare_timespec(&neigh->rx_exp_time, current_time) >= 0) {
            if(compare_timespec(&neigh->tx_exp_time, current_time) >= 0) {
                return BI_NEIGH;
            } else {
                return UNI_NEIGH;
            }
        } else {
            // assert( NE_isDeleted(neigh) ); // GC could have not expired yet so neigh is not deleted yet

            return LOST_NEIGH;
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

    /*
    double NE_getBiLinkQuality(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->bi_lq;
}
*/

void NE_setRxLinkQuality(NeighborEntry* neigh, double rx_lq) {
    assert(neigh);
    neigh->rx_lq = rx_lq;
}

void NE_setTxLinkQuality(NeighborEntry* neigh, double tx_lq) {
    assert(neigh);
    neigh->tx_lq = tx_lq;
}

/*
void NE_setBiLinkQuality(NeighborEntry* neigh, double bi_lq) {
assert(neigh);
neigh->bi_lq = bi_lq;
}
*/

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

double NE_getOutTraffic(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->out_traffic;
}

void NE_setOutTraffic(NeighborEntry* neigh, double traffic) {
    assert(neigh);
    neigh->out_traffic = traffic;
}

/*
unsigned int NE_getNeighborAttributesSize(NeighborEntry* neigh) {
assert(neigh);
return neigh->attributes_size;
}
*/

void* NE_getMessageAttributes(NeighborEntry* neigh) {
    assert(neigh);
    return neigh->msg_attributes;
}

void* NE_setMessageAttributes(NeighborEntry* neigh, void* msg_attributes) {
    assert(neigh);
    void* old_attributes = neigh->msg_attributes;
    neigh->msg_attributes = msg_attributes;
    return old_attributes;
}

/*
Window* NE_getMisses(NeighborEntry* neigh) {
assert(neigh);

return neigh->misses;
}
*/



/*
YggMessage* NE_getNeighborLastAnnounce(NeighborEntry* neigh) {
assert(neigh);
return neigh->last_announce;
}
*/

/*
void NE_setNeighborLastAnnounce(NeighborEntry* neigh, YggMessage* announce) {
assert(neigh);
if(neigh->last_announce == NULL && announce != NULL) {
neigh->last_announce = malloc(sizeof(YggMessage));
memcpy(neigh->last_announce, announce, sizeof(YggMessage));
} else if(neigh->last_announce != NULL && announce == NULL) {
free(neigh->last_announce);
neigh->last_announce = NULL;
} else {
memcpy(neigh->last_announce, announce, sizeof(YggMessage));
}
}
*/

/*
void NE_updateNeighborExpirationDate(NeighborEntry* neigh, struct timespec* exp) {
assert(neigh);
memcpy(&neigh->expiration_date, exp, sizeof(struct timespec));
}
*/

/*
void NE_updateNeighborVersion(NeighborEntry* neigh, unsigned short version) {
assert(neigh);
assert(neigh->version <= version);
neigh->version = version;
}
*/

/*typedef struct _NeighsIterator {
int index;
double_list_item* next_it;
} NeighsIterator;

NeighborEntry* NT_nextNeighbor(NeighborsTable* nt, void** iterator) {
NeighsIterator** it_ = (NeighsIterator**)iterator;
if(*it_ == NULL) {
*it_ = malloc(sizeof(NeighsIterator));
(*it_)->index = 0;
(*it_)->next_it = nt->nt->array_size > 0 ? nt->nt->array[0]->head : NULL;
}

NeighsIterator* it = *it_;

for(int i = it->index; i < nt->nt->array_size; i++) {
double_list_item* dli = it->index == i ? it->next_it : nt->nt->array[i]->head;
for(; dli; dli = dli->next) {
it->index = i;
it->next_it = dli->next;

//printf("iterator [%d/%d] %d\n", i, nt->nt->array_size, nt->nt->array[i]->size);

return (NeighborEntry*)(((hash_table_item*)dli->data)->value);
}
}

free(it);
*iterator = NULL;
return NULL;
}*/

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

    hash_table_item* hit = hash_table_remove(neigh->neighs, nn_id);
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

/*bool addNeighborNeigh(NeighborEntry* neigh, unsigned char* nn_id, unsigned short seq, bool is_bi, double rx_lq, double tx_lq, double traffic, struct timespec* expiration) {
assert(neigh);

bool new_or_updated = false;

TwoHopNeighborEntry* nn = hash_table_find_value(neigh->neighs, nn_id);
if( nn == NULL ) {
nn = newTwoHopNeighborEntry(nn_id, seq, is_bi, rx_lq, tx_lq, traffic, expiration);

hash_table_insert(neigh->neighs, nn->id, nn);

new_or_updated = true;
} else {
if( seq >= nn->seq ) {
nn->seq = seq;

if( nn->is_bi != is_bi ) {
nn->is_bi = is_bi;
new_or_updated = true;
}

if( fabs(nn->rx_lq - rx_lq) > 0.0 || fabs(nn->tx_lq - tx_lq) > 0.0 ) {
new_or_updated = true;
}
nn->rx_lq = rx_lq;
nn->tx_lq = tx_lq;

if( fabs(nn->traffic - traffic) > 0.0 ) {
new_or_updated = true;
}
nn->traffic = traffic;

//
//if( compare_timespec(&nn->expiration, expiration) > 0 ) {
//    new_or_updated = true;
//}
copy_timespec(&nn->expiration, expiration);
} else {
new_or_updated = false;
}
}

return new_or_updated;
}*/

/*
bool removeNeighborNeigh(NeighborEntry* neigh, unsigned char* nn_id) {
assert(neigh);

hash_table_item* hit = hash_table_remove(neigh->neighs, nn_id);
if(hit) {
free(hit->value);
free(hit);
return true;
} else {
return false;
}
}
*/


char* NT_print(NeighborsTable* nt, char** str, struct timespec* current_time, unsigned char* myID, WLANAddr* myMAC, unsigned short my_seq) {
    char* header = " # |                 ID                   |"
    "        MAC        |  SEQ  | T |"
    "   PERIODS   |"
    " RX_LQ | TX_LQ | TRAFFIC | RX_EXP | TX_EXP | FOUND  |  LOST  | REMOVE | NEIGHS \n";

    unsigned int line_size = strlen(header) + 1;

    /*
    char line_str[line_size];
    memset(line_str, 196, line_size);
    line_str[line_size-1] = '\0';
    */

    unsigned int nneighs = 0;
    void* iterator = NULL;
    for(NeighborEntry* current_neigh = NT_nextNeighbor(nt, &iterator); current_neigh; current_neigh = NT_nextNeighbor(nt, &iterator)) {
        nneighs = lMax(nneighs, NE_getTwoHopNeighbors(current_neigh)->n_items);
    }

    unsigned long buffer_size = NT_getSize(nt)*(line_size*(nneighs+1)) + 3*line_size;

    char* buffer = malloc(buffer_size);
    char* ptr = buffer;

    // Print myself
    /*char id_str[UUID_STR_LEN+1];
    uuid_unparse(myID, id_str);
    id_str[UUID_STR_LEN] = '\0';

    char addr_str[20];
    wlan2asc(myMAC, addr_str);
    align_str(addr_str, addr_str, 17, "CL");


    double out_traffic = computeWindow(NT_getOutTraffic(neighbors), current_time, window_type, "sum", true);

    double in_traffic = 0.0;
    iterator = NULL;
    for(NeighborEntry* current_neigh = NT_nextNeighbor(neighbors, &iterator); current_neigh; current_neigh = NT_nextNeighbor(neighbors, &iterator)) {
    in_traffic += NE_getOutTraffic(current_neigh);
}

double instability = computeWindow(NT_getInstability(neighbors), current_time, window_type, "sum", true);

sprintf(ptr, "%s    %s    %hu    out: %0.3f msgs/s in: %0.3f msgs/s    %0.3f churn    %u neighbors \n", id_str, addr_str, my_seq, out_traffic, in_traffic, instability, NT_getSize(neighbors));
ptr += strlen(ptr);
*/

// Print Column Headers
sprintf(ptr, "%s", header);
ptr += strlen(ptr);

// Print Line
/*
sprintf(ptr, "%s\n", line_str);
ptr += strlen(ptr);
*/

// Print each neighbor
unsigned int counter = 0;
iterator = NULL;
for(NeighborEntry* current_neigh = NT_nextNeighbor(nt, &iterator); current_neigh; current_neigh = NT_nextNeighbor(nt, &iterator)) {

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
        case UNI_NEIGH: type_str = "U"; break;
        case BI_NEIGH: type_str = "B"; break;
        case LOST_NEIGH:
        default: type_str = "L";
    }

    struct timespec aux_t;

    // bool bi_available = true;

    char remove_str[7];
    char lost_str[7];

    char rx_exp_str[7];
    char rx_lq_str[6];
    if(NE_isDeleted(current_neigh)) {
        sprintf(rx_lq_str, "  -  ");

        sprintf(rx_exp_str, "   -  ");

        subtract_timespec(&aux_t, NE_getNeighborRemovalTime(current_neigh), current_time);
        timespec_to_string(&aux_t, remove_str, 6, 1);
        align_str(remove_str, remove_str, 6, "CR");

        subtract_timespec(&aux_t, current_time, NE_getNeighborDeletedTime(current_neigh));
        timespec_to_string(&aux_t, lost_str, 6, 1);
        align_str(lost_str, lost_str, 6, "CR");

        // bi_available = false;
    } else {
        sprintf(rx_lq_str, "%0.3f", NE_getRxLinkQuality(current_neigh));

        subtract_timespec(&aux_t, NE_getNeighborRxExpTime(current_neigh), current_time);
        timespec_to_string(&aux_t, rx_exp_str, 6, 1);
        align_str(rx_exp_str, rx_exp_str, 6, "CR");

        sprintf(remove_str, "   -  ");

        sprintf(lost_str, "   -  ");
    }

    char tx_exp_str[7];
    char tx_lq_str[6];
    if(compare_timespec(NE_getNeighborTxExpTime(current_neigh), current_time) < 0) {
        sprintf(tx_exp_str, "   -  ");

        sprintf(tx_lq_str, "  -  ");

        // bi_available = false;
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


    sprintf(ptr, "%2.d   %s   %s   %s   %s   %s   %s   %s   "
    " %s    %s   %s   %s   %s   %s   %s \n",
    counter+1, id_str, addr_str, seq_str, type_str, periods_str, rx_lq_str, tx_lq_str, traffic_str, rx_exp_str, tx_exp_str, found_str, lost_str, remove_str, neighs_str);
    ptr += strlen(ptr);

    // TODO: 2-hop neighs
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
        sprintf(tx_lq_str, "%0.3f", nneigh->tx_lq);

        subtract_timespec(&aux_t, &nneigh->expiration, current_time);
        timespec_to_string(&aux_t, rx_exp_str, 6, 1);
        align_str(rx_exp_str, rx_exp_str, 6, "CR");

        sprintf(traffic_str, "%0.3f" , nneigh->traffic);

        char c = uuid_compare(myID, nneigh->id) == 0 ? '*' : ' ';

        sprintf(ptr, "     %c   %s                   %s   %s                 %s   %s    %s             %s  \n",
        c, id_str, seq_str, type_str, rx_lq_str, tx_lq_str, traffic_str, rx_exp_str);
        ptr += strlen(ptr);
    }

    counter++;
}

assert(ptr <= buffer+buffer_size);

*str = buffer;
return *str;
}
