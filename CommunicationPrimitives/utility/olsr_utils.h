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

#ifndef _OLSR_UTILS_H_
#define _OLSR_UTILS_H_

#include "data_structures/hash_table.h"
#include "my_misc.h"

#define DEFAULT_WILLINGNESS 1

typedef struct N1_Tuple_ {
    uuid_t id;
    double d1;
    double w;
    list* ns;
    bool already_mpr;
} N1_Tuple;

typedef struct N2_Tuple_ {
    uuid_t id;
    double d2;
} N2_Tuple;

bool equalN2Tuple(void* a, void* b);

N1_Tuple* newN1Tuple(unsigned char* id, double d1, double w, list* ns, bool already_mpr);

N2_Tuple* newN2Tuple(unsigned char* id, double d2);

void destroyN1(hash_table* n1);

void destroyN2(list* n2);

list* compute_multipoint_relays(hash_table* n1, list* n2, list* initial);

void delete_n1_item(hash_table_item* hit, void* aux);

/////////////////////////////////////////

typedef struct DijkstraTuple_ {
    uuid_t destination_id;
    uuid_t next_hop_id;
    double cost;
    unsigned int hops;
} DijkstraTuple;

DijkstraTuple* newDijkstraTuple(unsigned char* dest_id, unsigned char* next_hop_id, double cost, unsigned int hops);

hash_table* Dijkstra(graph* g, unsigned char* source_id);

#endif /* _OLSR_UTILS_H_ */
