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

#ifndef DATA_STRUCTURES_LIST_H_
#define DATA_STRUCTURES_LIST_H_

// This file consists in an extension to the following list:
#include "Yggdrasil/data_structures/generic/list.h"

void list_append(list* l1, list* l2);

void list_delete(list* l);

void list_delete_keep(list* l);

bool list_contained(list* l1, list* l2, comparator_function cmp, bool orEqual);

// Order does not matter
bool list_equal(list* l1, list* l2, comparator_function cmp);

bool list_is_empty(list* l);

list* list_clone(list* l, unsigned int data_size);

list* list_intercept(list* l1, list* l2, comparator_function cmp, unsigned int data_size);

list* list_difference(list* l1, list* l2, comparator_function cmp, unsigned int data_size);

list* list_map(list* l, void* (*f)(void* v, unsigned int argc, void** argv), unsigned int argc, void** argv);

list* new_list(unsigned int n, ...);

#endif /* DATA_STRUCTURES_LIST_H_ */
