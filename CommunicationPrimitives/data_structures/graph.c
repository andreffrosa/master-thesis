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

#include "graph.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define DEFAULT_VALUE_SIZE 0
#define DEFAULT_LABEL_SIZE 0

typedef struct _pair {
    void* x;
    void* y;
} pair;

static bool equal_nodes(void* node, void* aux) {
    void* key = ((pair*)aux)->x;
    key_comparator compare = ((pair*)aux)->y;
    return compare(((graph_node*)node)->key, key) == 0;
}

static bool equal_keys(void* key1, void* aux) {
    void* key2 = ((pair*)aux)->x;
    key_comparator compare = ((pair*)aux)->y;
    return compare(key1, key2) == 0;
}

static bool equal_edges(void* edge, void* aux) {
    void* start = ((pair*)((pair*)aux)->x)->x;
    void* end = ((pair*)((pair*)aux)->x)->y;
    key_comparator compare = ((pair*)aux)->y;
    return compare(((graph_edge*)edge)->start_node->key, start) == 0
        && compare(((graph_edge*)edge)->end_node->key, end) == 0;
}

graph* graph_init(key_comparator compare, unsigned int key_size) {
    return graph_init_complete(compare, NULL, NULL, key_size, DEFAULT_VALUE_SIZE, DEFAULT_LABEL_SIZE);
}

graph* graph_init_complete(key_comparator compare, delete_node del_node, delete_edge del_edge, unsigned int key_size, unsigned int value_size, unsigned int label_size) {
    graph* g = malloc(sizeof(struct _graph));

    g->nodes = list_init();
    g->edges = list_init();

	g->key_cmp = compare;
	g->del_node = del_node;
    g->del_edge = del_edge;

    g->key_size = key_size;
    g->value_size = value_size;
    g->label_size = label_size;

    return g;
}

void* graph_insert_node(graph* g, void* key, void* value) {
    assert(g);
    assert(key);

	void* result = NULL;

    graph_node* node = graph_find_node(g, key);
	if( node != NULL ) {
        result = node->value;
        node->value = value;

        // Leave the adjacencies

	} else {
        node = malloc(sizeof(graph_node));

        node->key = key;
        node->value = value;
        node->in_adjacencies = list_init();
        node->out_adjacencies = list_init();

        list_add_item_to_tail(g->nodes, node);
	}

	return result;
}

void* graph_insert_edge(graph* g, void* start, void* end, void* label) {
    assert(g);
    assert(start && end && start != end);

    graph_node* node1 = graph_find_node(g, start);
    graph_node* node2 = graph_find_node(g, end);

    assert(node1 != NULL && node2 != NULL && node1 != node2);

	// Verify if the edge already exists
    graph_edge* edge = graph_find_edge(g, start, end);

    void* result = NULL;

    if(edge != NULL) {
        result = edge->label;
    } else {
        edge = malloc(sizeof(graph_edge));

        edge->start_node = node1;
        edge->end_node = node2;

        list_add_item_to_tail(g->edges, edge);
    }

    edge->label = label;

    list_add_item_to_tail(node1->out_adjacencies, edge);
    list_add_item_to_tail(node2->in_adjacencies, edge);

    return result;
}

graph_node* graph_find_node(graph* g, void* key) {
    assert(g);
    assert(key);

    pair aux = (pair){key, g->key_cmp};
    graph_node* node = (graph_node*)list_find_item(g->nodes, &equal_nodes, &aux);
    return node;
}

graph_edge* graph_find_edge(graph* g, void* start, void* end) {
    assert(g);
    assert(start && end && start != end);

    pair aux1 = (pair){start, end};
    pair aux2 = (pair){&aux1, g->key_cmp};
    graph_edge* edge = (graph_edge*)list_find_item(g->edges, &equal_edges, &aux2);
    return edge;
}

void* graph_find_value(graph* g, void* key) {
    assert(g);
    assert(key);

    graph_node* node = graph_find_node(g, key);
    return node ? node->value : NULL;
}

int graph_get_node_in_degree(graph* g, void* key) {
    assert(g);
    assert(key);

    graph_node* node = graph_find_node(g, key);
    return node ? node->in_adjacencies->size : -1;
}

int graph_get_node_out_degree(graph* g, void* key) {
    assert(g);
    assert(key);

    graph_node* node = graph_find_node(g, key);
    return node ? node->out_adjacencies->size : -1;
}

void* graph_find_label(graph* g, void* start, void* end) {
    assert(g);
    assert(start && end && start != end);

    graph_edge* edge = graph_find_edge(g, start, end);
    return edge ? edge->label : NULL;
}

void* graph_remove_edge(graph* g, void* start, void* end) {
    assert(g);
    assert(start && end && start != end);

    pair aux1 = (pair){start, end};
    pair aux2 = (pair){&aux1, g->key_cmp};
    graph_edge* edge = (graph_edge*)list_remove_item(g->edges, &equal_edges, &aux2);

    void* result = NULL;
    if(edge != NULL) {
        list_remove_item(edge->start_node->out_adjacencies, &equal_edges, &aux2);
        list_remove_item(edge->end_node->in_adjacencies, &equal_edges, &aux2);

        result = edge->label;
        edge->label = NULL;
        graph_delete_edge(edge, NULL);
    }

    return result;
}

void* graph_remove_node(graph* g, void* key) {
    assert(g);
    assert(key);

    pair aux = (pair){key, g->key_cmp};
    graph_node* node = (graph_node*)list_remove_item(g->nodes, &equal_nodes, &aux);

    void* result = NULL;
    if(node != NULL) {
        graph_edge* edge = NULL;
        while( (edge = (graph_edge*)list_remove_head(node->in_adjacencies)) ) {
            void* res = graph_remove_edge(g, edge->start_node->key, edge->end_node->key);
            if(res != NULL) {
                if(g->del_edge != NULL) {
                    g->del_edge(res);
                }
            }
        }
        //free(node->in_adjacencies);

        while( (edge = (graph_edge*)list_remove_head(node->out_adjacencies)) ) {
            void* res = graph_remove_edge(g, edge->start_node->key, edge->end_node->key);
            if(res != NULL) {
                if(g->del_edge != NULL) {
                    g->del_edge(res);
                }
            }
        }
        //free(node->out_adjacencies);

        result = node->value;
        node->value = NULL;
        graph_delete_node(node, NULL);
    }

    return result;
}

void graph_delete_node(graph_node* node, delete_node del_node) {
    assert(node);
    assert(node->key != NULL);

    if(node->key == node->value) {
		free(node->key);
	} else {
		if(node->key != NULL)
			free(node->key);
		if(node->value != NULL) {
            if(del_node != NULL) {
                del_node(node->value);
            }
            free(node->value);
        }
	}

    list_delete_keep(node->in_adjacencies);
    list_delete_keep(node->out_adjacencies);

    free(node);
}

void graph_delete_edge(graph_edge* edge, delete_edge del_edge) {
    assert(edge);
    assert(edge->start_node && edge->end_node);

    if(edge->label != NULL) {
        if(del_edge != NULL)
            del_edge(edge->label);

        free(edge->label);
    }
    free(edge);
}

void graph_delete(graph* g) {

    if(g == NULL)
        return;

    // Delete nodes
    graph_node* node = NULL;
    while( (node = (graph_node*)list_remove_head(g->nodes)) ) {
        graph_delete_node(node, g->del_node);
    }
    free(g->nodes);

    // Delete edges
    graph_edge* edge = NULL;
    while( (edge = (graph_edge*)list_remove_head(g->edges)) ) {
        graph_delete_edge(edge, g->del_edge);
    }
    free(g->edges);

	free(g);
}

list* graph_get_adjacencies(graph* g, void* key, AdjacencyType adj_type) {
    assert(g);
    assert(key);

    graph_node* node = graph_find_node(g, key);

    /*if(node == NULL)
        return NULL;

    list* result = list_init();

    list* l = in_or_out ? node->in_adjacencies : node->out_adjacencies;

    for(list_item* it = l->head; it; it = it->next) {
        void* aux = malloc(g->key_size);
        memcpy(aux, it->data, g->key_size);
        list_add_item_to_tail(result, aux);
    }

	return result;*/

    return graph_get_adjacencies_from_node(g, node, adj_type);
}


static void* append_cmp(void* v, unsigned int argc, void** argv) {
    assert(argc == 1);
    key_comparator g = (key_comparator)argv[0];

    pair* p = malloc(sizeof(pair));
    p->x = v;
    p->y = g;
    return (void*)p;
}

list* graph_get_adjacencies_from_node(graph* g, graph_node* node, AdjacencyType adj_type) {
    assert(g);

    if(node == NULL)
        return NULL;

    if(adj_type == IN_ADJ || adj_type == OUT_ADJ) {
        list* l = NULL;
        switch (adj_type) {
            case IN_ADJ: l = node->in_adjacencies; break;
            case OUT_ADJ: l = node->out_adjacencies; break;
            default: assert(false);
        }

        list* result = list_init();
        for(list_item* it = l->head; it; it = it->next) {
            graph_edge* edge = (graph_edge*)it->data;
            void* aux = malloc(g->key_size);

            switch (adj_type) {
                case IN_ADJ: memcpy(aux, edge->start_node->key, g->key_size); break;
                case OUT_ADJ: memcpy(aux, edge->end_node->key, g->key_size); break;
                default: assert(false);
            }

            list_add_item_to_tail(result, aux);
        }

        return result;
    } else { // SYM_ADJ
        list* aux1 = graph_get_adjacencies_from_node(g, node, IN_ADJ);
        list* aux2 = graph_get_adjacencies_from_node(g, node, OUT_ADJ);

        list* aux3 = list_map(aux1, &append_cmp, 1, (void*[]){(void*)g->key_cmp});
        list* result = list_intercept(aux3, aux2, &equal_keys, g->key_size);

        list_delete(aux1);
        list_delete(aux2);
        list_delete(aux3);

        return result;
    }
}

graph* graph_clone(graph* g) {
    assert(g);

    graph* cl = graph_init_complete(g->key_cmp, g->del_node, g->del_edge, g->key_size, g->value_size, g->label_size);

    // Copy nodes
    for(list_item* it = g->nodes->head; it; it = it->next) {
        graph_node* current_node = (graph_node*)it->data;

        void* key = malloc(g->key_size);
        memcpy(key, current_node->key, g->key_size);

        void* value = NULL;
        if(current_node->key != NULL) {
            value = malloc(g->value_size);
            memcpy(value, current_node->value, g->value_size);
        }

        graph_insert_node(cl, key, value);
    }

    // Copy edges
    for(list_item* it = g->edges->head; it; it = it->next) {
        graph_edge* current_edge = (graph_edge*)it->data;

        void* label = NULL;
        if(current_edge->label != NULL) {
            if(g->label_size > 0) {
                label = malloc(g->label_size);
                memcpy(label, current_edge->label, g->label_size);
            } else {
                label = NULL;
            }
        }

        graph_insert_edge(cl, current_edge->start_node->key, current_edge->end_node->key, label);
    }

    return cl;
}
