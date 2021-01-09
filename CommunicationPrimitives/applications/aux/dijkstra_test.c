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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uuid/uuid.h>

#include "utility/olsr_utils.h"

#define ID_TEMPLATE "66600666-1001-1001-1001-000000000001"

static void parseNode(unsigned char* id, unsigned int node_id);
static graph* create_topology();

int main(int argc, char* argv[]) {

    graph* g = create_topology();

    uuid_t source;
    parseNode(source, 1);

    hash_table* routes = Dijkstra(g, source);

    printf("routes: %d\n", routes->n_items);

    printf("       DEST     |     NEXT HOP     | COST | HOPS \n");

    void* iterator = NULL;
    hash_table_item* it = NULL;
    while( (it = hash_table_iterator_next(routes, &iterator)) ) {
        DijkstraTuple* dt = (DijkstraTuple*)it->value;

        char dest_str[UUID_STR_LEN+1];
        dest_str[UUID_STR_LEN] = '\0';
        uuid_unparse(dt->destination_id, dest_str);

        char next_hop_str[UUID_STR_LEN+1];
        next_hop_str[UUID_STR_LEN] = '\0';
        uuid_unparse(dt->next_hop_id, next_hop_str);

        printf("%s %s %f %u\n", dest_str, next_hop_str, dt->cost, dt->hops);
    }

    graph_delete(g);

    return 0;
}

static void parseNode(unsigned char* id, unsigned int node_id) {
    char id_str[UUID_STR_LEN+1];
    strcpy(id_str, ID_TEMPLATE);

    char aux[10];
    sprintf(aux, "%u", node_id);
    int len = strlen(aux);

    char* ptr = id_str + strlen(id_str) - len;
    memcpy(ptr, aux, len);

    int a = uuid_parse(id_str, id);
    assert(a >= 0);
}

static unsigned char* getNode(uuid_t nodes[], unsigned int id) {
    return nodes[id-1];
}

static graph* create_topology() {

    graph* g = graph_init_complete((key_comparator)&uuid_compare, NULL, NULL, sizeof(uuid_t), 0, sizeof(double));

    unsigned int amount = 9;
    uuid_t nodes[amount];
    for(int i = 0; i < amount; i++) {
        parseNode(nodes[i], i+1);

        graph_insert_node(g, new_id(nodes[i]), NULL);
    }

    graph_insert_edge(g, getNode(nodes, 1), getNode(nodes, 2), new_double(1));
    graph_insert_edge(g, getNode(nodes, 2), getNode(nodes, 1), new_double(1));

    graph_insert_edge(g, getNode(nodes, 1), getNode(nodes, 3), new_double(1));
    graph_insert_edge(g, getNode(nodes, 3), getNode(nodes, 1), new_double(1));

    graph_insert_edge(g, getNode(nodes, 2), getNode(nodes, 3), new_double(1));
    graph_insert_edge(g, getNode(nodes, 3), getNode(nodes, 2), new_double(1));

    graph_insert_edge(g, getNode(nodes, 3), getNode(nodes, 4), new_double(1));
    graph_insert_edge(g, getNode(nodes, 4), getNode(nodes, 3), new_double(1));

    graph_insert_edge(g, getNode(nodes, 4), getNode(nodes, 5), new_double(1));
    graph_insert_edge(g, getNode(nodes, 5), getNode(nodes, 4), new_double(1));

    graph_insert_edge(g, getNode(nodes, 5), getNode(nodes, 6), new_double(1));
    graph_insert_edge(g, getNode(nodes, 6), getNode(nodes, 5), new_double(1));

    graph_insert_edge(g, getNode(nodes, 5), getNode(nodes, 8), new_double(1));
    graph_insert_edge(g, getNode(nodes, 8), getNode(nodes, 5), new_double(1));

    graph_insert_edge(g, getNode(nodes, 6), getNode(nodes, 7), new_double(1));
    graph_insert_edge(g, getNode(nodes, 7), getNode(nodes, 6), new_double(1));

    graph_insert_edge(g, getNode(nodes, 7), getNode(nodes, 8), new_double(1));
    graph_insert_edge(g, getNode(nodes, 8), getNode(nodes, 7), new_double(1));

    graph_insert_edge(g, getNode(nodes, 9), getNode(nodes, 8), new_double(1));
    graph_insert_edge(g, getNode(nodes, 8), getNode(nodes, 9), new_double(1));

    return g;
}
