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

 #ifndef MY_MISC_H_
 #define MY_MISC_H_

#include "Yggdrasil.h"

#include "data_structures/graph.h"

#define INT_STR_LEN 12
#define LONG_STR_LEN 21

/*typedef struct _Tuple {
    unsigned int size;
    void** entries;
} Tuple;

Tuple* newTuple(void** entries, unsigned int size);
*/

void insertionSort(void* v, unsigned int element_size, unsigned int n_elements, int (*cmp)(void*,void*));

bool equalID(void* a, void* b);

unsigned long uuid_hash(unsigned char* id);

bool equalInt(void* a, void* b);

unsigned long int_hash(int* n);

bool equalAddr(void* a, void* b);

void pushMessageType(YggMessage* msg, unsigned char type);

unsigned char popMessageType(YggMessage* msg);



/*list* get_bidirectional_stable_neighbors(graph* neighborhood, unsigned char* id);

list* compute_mprs(graph* neighborhood, unsigned char* myID);

*/

int is_memory_zero(const void* addr, unsigned long size);

topology_manager_args* load_overlay(char* overlay_path, char* hostname);

typedef struct ModuleState_ {
    void* args;
    void* vars;
} ModuleState;

bool is_unicast_message(YggMessage* msg);

unsigned char* new_id(unsigned char* id);

double* new_double(double d);

#endif /* MY_MISC_H_ */
