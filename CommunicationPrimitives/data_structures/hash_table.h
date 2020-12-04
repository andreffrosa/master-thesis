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

#ifndef DATA_STRUCTURES_HASH_TABLE_H_
#define DATA_STRUCTURES_HASH_TABLE_H_

#include <stdlib.h>

#include "double_list.h"

typedef unsigned long (*hashing_function)(void* key);

typedef struct _hash_table {
	double_list** array;
	unsigned int array_size;
	unsigned int n_items;
	hashing_function hash_fun;
	comparator_function comp_fun;
} hash_table;

typedef struct _hash_table_item {
	void* key;
	void* value;
	comparator_function cmp;
} hash_table_item;

hash_table* hash_table_init_size(unsigned int size, hashing_function hash_fun, comparator_function comp_fun);

hash_table* hash_table_init(hashing_function hash_fun, comparator_function comp_fun);

hash_table_item* hash_table_find_item(hash_table* table, void* key);

void* hash_table_find_value(hash_table* table, void* key);

void* hash_table_insert(hash_table* table, void* key, void* value);

hash_table_item* hash_table_remove_item(hash_table* table, void* key);

void* hash_table_remove(hash_table* table, void* key);

void hash_table_delete(hash_table* table);

void hash_table_delete_custom(hash_table* table, void (*delete_item)(hash_table_item*,void*), void* args);

hash_table_item* hash_table_iterator_next(hash_table* table, void** iterator);

hash_table* hash_table_clone(hash_table* table, unsigned int key_size, unsigned int value_size);

#endif /* DATA_STRUCTURES_HASH_TABLE_H_ */
