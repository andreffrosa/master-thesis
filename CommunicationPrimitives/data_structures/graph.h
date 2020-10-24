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

#ifndef DATA_STRUCTURES_GRAPH_H_
#define DATA_STRUCTURES_GRAPH_H_

#include "list.h"

typedef struct _graph_node {
	void* key;
	void* value;
	list* in_adjacencies;
    list* out_adjacencies;
} graph_node;

typedef struct _graph_edge {
    graph_node* start_node;
    graph_node* end_node;
    void* label;
} graph_edge;

typedef int (*key_comparator)(void* k1, void* k2);
typedef void (*delete_node)(graph_node*);
typedef void (*delete_edge)(graph_edge*);

typedef struct _graph {
    list* nodes;
    list* edges;

	key_comparator key_cmp;
	delete_node del_node;
    delete_edge del_edge;

    unsigned int key_size;
    unsigned int value_size;
    unsigned int label_size;
} graph;

graph* graph_init(key_comparator compare, unsigned int key_size);

graph* graph_init_complete(key_comparator compare, delete_node del_node, delete_edge del_edge, unsigned int key_size, unsigned int value_size, unsigned int label_size);

void* graph_insert_node(graph* g, void* key, void* value);

void* graph_insert_edge(graph* g, void* start, void* end, void* label);

// void* graph_insert(graph* g, void* key, void* value, list* adjacencies);

graph_node* graph_find_node(graph* g, void* key);

graph_edge* graph_find_edge(graph* g, void* start, void* end);

void* graph_find_value(graph* g, void* key);

int graph_get_node_in_degree(graph* g, void* key);

int graph_get_node_out_degree(graph* g, void* key);

void* graph_find_label(graph* g, void* start, void* end);

void* graph_remove_node(graph* g, void* key);

void* graph_remove_edge(graph* g, void* start, void* end);

void graph_delete_node(graph_node* node, delete_node del_node);

void graph_delete_edge(graph_edge* edge, delete_edge del_edge);

void graph_delete(graph* g);

typedef enum {
    IN_ADJ,
    OUT_ADJ,
    SYM_ADJ
} AdjacencyType;

list* graph_get_adjacencies(graph* g, void* key, AdjacencyType adj_type);

list* graph_get_adjacencies_from_node(graph* g, graph_node* node, AdjacencyType adj_type);

graph* graph_clone(graph* g);

#endif /* DATA_STRUCTURES_GRAPH_H_ */
