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

#include "retransmission_context_private.h"

#include "protocols/discovery/framework/framework.h"
#include "data_structures/graph.h"
#include "data_structures/list.h"
#include "utility/my_misc.h"
#include "utility/my_math.h"
#include "utility/my_time.h"

#include <assert.h>

typedef struct _NeighborsContextState {
	graph* neighborhood;

    double in_traffic;
    double out_traffic;
    double new_neighbors_flux;
    double lost_neighbors_flux;
    //unsigned int n_neighbors;
    double neighbors_density;
} NeighborsContextState;

/*typedef struct _NeighborsContextArgs {
	//topology_discovery_args* d_args;
	//bool append_neighbors;
} NeighborsContextArgs;*/

static graph* extractNeighborhood(YggEvent* ev);
static void updateEnvironment(NeighborsContextState* state, YggEvent* ev);

static unsigned int minMaxNeighs(graph* neighborhood, unsigned char* myID, bool max, AdjacencyType adj_type);
static double avgNeighs(graph* neighborhood, unsigned char* myID, bool include_me, AdjacencyType adj_type);
static void getneighborsDistribution(graph* neighborhood, unsigned char* myID, double* result);
static list* getneighborsInCommon(graph* neighborhood, uuid_t neigh1_id, uuid_t neigh2_id);
static list* getCoverage(graph* neighborhood, unsigned char* myID, double_list* copies, bool onlyFirst);
static list* notCovered(graph* neighborhood, unsigned char* myID, double_list* copies);
static bool allCovered(graph* neighborhood, unsigned char* myID, double_list* copies);

static void NeighborsContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
    proto_def_add_consumed_event(protocol_definition, DISCOVERY_FRAMEWORK_PROTO_ID, NEW_NEIGHBOR);
    proto_def_add_consumed_event(protocol_definition, DISCOVERY_FRAMEWORK_PROTO_ID, UPDATE_NEIGHBOR);
    proto_def_add_consumed_event(protocol_definition, DISCOVERY_FRAMEWORK_PROTO_ID, LOST_NEIGHBOR);
    proto_def_add_consumed_event(protocol_definition, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBORHOOD);
    proto_def_add_consumed_event(protocol_definition, DISCOVERY_FRAMEWORK_PROTO_ID, GENERIC_DISCOVERY_EVENT);
    proto_def_add_consumed_event(protocol_definition, DISCOVERY_FRAMEWORK_PROTO_ID, DISCOVERY_ENVIRONMENT_UPDATE);

    NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);

    unsigned char* my_id = malloc(sizeof(uuid_t));
    uuid_copy(my_id, myID);
    graph_insert_node(state->neighborhood, my_id, NULL);
}

static void NeighborsContextEvent(ModuleState* context_state, queue_t_elem* elem, unsigned char* myID, list* visited) {
    NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);

    if(elem->type == YGG_EVENT) {
        YggEvent* ev = &elem->data.event;
        unsigned short nid = ev->notification_id;
		if(nid == NEIGHBORHOOD) {
            graph_delete(state->neighborhood);
            state->neighborhood = extractNeighborhood(ev);
		/*} else if(elem->data.event.notification_id == GENERIC_DISCOVERY_EVENT) {
		*/
        } else if(elem->data.event.notification_id == DISCOVERY_ENVIRONMENT_UPDATE) {
            updateEnvironment(state, ev);
        }
	}
}

static bool NeighborsContextQuery(ModuleState* context_state, const char* query, void* result, hash_table* query_args, unsigned char* myID, list* visited) {
	NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);

	if(strcmp(query, "graph") == 0 || strcmp(query, "neighborhood") == 0) {
		*((graph**)result) = graph_clone(state->neighborhood);
		return true;
	} else if(strcmp(query, "degree") == 0 ) {
        unsigned char* id = query_args ? hash_table_find_value(query_args, "id") : NULL;
        id = id ? id : myID;

        list* sym_neighs = graph_get_adjacencies(state->neighborhood, id, SYM_ADJ);
        int deg = sym_neighs->size;
        list_delete(sym_neighs);

        *((unsigned int*)result) = deg;
		return true;
	} else if(strcmp(query, "in_degree") == 0 || strcmp(query, "n_neighbors") == 0) {
        unsigned char* id = query_args ? hash_table_find_value(query_args, "id") : NULL;
        id = id ? id : myID;

		int deg = graph_get_node_in_degree(state->neighborhood, id);
		assert(deg >= 0);

        *((unsigned int*)result) = deg;
		return true;
	} else if(strcmp(query, "out_degree") == 0) {
        unsigned char* id = query_args ? hash_table_find_value(query_args, "id") : NULL;
        id = id ? id : myID;

		int deg = graph_get_node_out_degree(state->neighborhood, id);
		assert(deg >= 0);

        *((unsigned int*)result) = deg;
		return true;
	} else if(strcmp(query, "in_neighbors") == 0 || strcmp(query, "neighbors") == 0 ) {
        unsigned char* id = query_args ? hash_table_find_value(query_args, "id") : NULL;
        id = id ? id : myID;

        *((list**)result) = graph_get_adjacencies(state->neighborhood, id, IN_ADJ);
		return true;
	} else if(strcmp(query, "out_neighbors") == 0 ) {
        unsigned char* id = query_args ? hash_table_find_value(query_args, "id") : NULL;
        id = id ? id : myID;

        *((list**)result) = graph_get_adjacencies(state->neighborhood, id, OUT_ADJ);
		return true;
	} else if(strcmp(query, "sym_neighbors") == 0 || strcmp(query, "bi_neighbors") == 0 ) {
        unsigned char* id = query_args ? hash_table_find_value(query_args, "id") : NULL;
        id = id ? id : myID;

        *((list**)result) = graph_get_adjacencies(state->neighborhood, id, SYM_ADJ);
		return true;
	} else if(strcmp(query, "max_neighbors") == 0) {
        AdjacencyType* aux = query_args ? hash_table_find_value(query_args, "adj_type") : NULL;
        AdjacencyType adj_type = aux ? *aux : IN_ADJ;

		*((unsigned int*)result) = minMaxNeighs(state->neighborhood, myID, true, adj_type);
		return true;
	} else if(strcmp(query, "min_neighbors") == 0) {
        AdjacencyType* aux = query_args ? hash_table_find_value(query_args, "adj_type") : NULL;
        AdjacencyType adj_type = aux ? *aux : IN_ADJ;

		*((unsigned int*)result) = minMaxNeighs(state->neighborhood, myID, false, adj_type);
		return true;
	} else if(strcmp(query, "avg_neighbors") == 0) {
        bool* aux = query_args ? hash_table_find_value(query_args, "include_me") : NULL;
        bool include_me = aux ? *aux : true;

		*((double*)result) = avgNeighs(state->neighborhood, myID, include_me, IN_ADJ);
		return true;
	} else if(strcmp(query, "neighbors_distribution") == 0) {
		getneighborsDistribution(state->neighborhood, myID, (double*)result);
		return true;
	} else if(strcmp(query, "coverage") == 0) {
        assert(query_args);
        void** aux1 = hash_table_find_value(query_args, "p_msg");
        assert(aux1);
		PendingMessage* p_msg = *aux1;
        assert(p_msg);

        char* aux2 = hash_table_find_value(query_args, "coverage");
        char* coverage = aux2 ? aux2 : "all";

        if(strcmp(coverage, "first")==0) {
            *((list**)result) = getCoverage(state->neighborhood, myID, getCopies(p_msg), true);
        } else if(strcmp(coverage, "all")==0) {
            *((list**)result) = getCoverage(state->neighborhood, myID, getCopies(p_msg), false);
        } else {
                return false;
        }
		return true;
	} else if(strcmp(query, "not_covered") == 0) {
        assert(query_args);
        void** aux1 = hash_table_find_value(query_args, "p_msg");
        assert(aux1);
		PendingMessage* p_msg = *aux1;
        assert(p_msg);

		*((list**)result) = notCovered(state->neighborhood, myID, getCopies(p_msg));
		return true;
	} else if(strcmp(query, "all_covered") == 0) {
        assert(query_args);
        void** aux1 = hash_table_find_value(query_args, "p_msg");
        assert(aux1);
		PendingMessage* p_msg = *aux1;
        assert(p_msg);

		*((bool*)result) = allCovered(state->neighborhood, myID, getCopies(p_msg));
		return true;
	} else if(strcmp(query, "in_traffic") == 0) {
        *((double*)result) = state->in_traffic;
		return true;
    } else if(strcmp(query, "out_traffic") == 0) {
        *((double*)result) = state->out_traffic;
        return true;
    } else if(strcmp(query, "new_neighbors_flux") == 0) {
        *((double*)result) = state->new_neighbors_flux;
        return true;
    } else if(strcmp(query, "lost_neighbors_flux") == 0) {
        *((double*)result) = state->lost_neighbors_flux;
        return true;
    } else if(strcmp(query, "neighbors_density") == 0) {
        *((double*)result) = state->neighbors_density;
        return true;
    } else {
		return false;
	}
}

static void NeighborsContextDestroy(ModuleState* context_state, list* visited) {

    NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);
    graph_delete(state->neighborhood);
    free(state);
}

RetransmissionContext* NeighborsContext() {

    NeighborsContextState* state = malloc(sizeof(NeighborsContextState));

    state->neighborhood = graph_init((key_comparator)&uuid_compare, sizeof(uuid_t));
    state->in_traffic = 0.0;
    state->out_traffic = 0.0;
    state->new_neighbors_flux = 0.0;
    state->lost_neighbors_flux = 0.0;
    state->neighbors_density = 0.0;

	return newRetransmissionContext(
        NULL,
        state,
        &NeighborsContextInit,
        &NeighborsContextEvent,
        NULL,
        &NeighborsContextQuery,
        NULL,
        NULL,
        &NeighborsContextDestroy
    );
}

/*

static void newNeighbor(graph* neighborhood, unsigned char* myID, YggEvent* ev) {
    assert(ev && ev->payload && ev->length > 0);

    void* ptr = NULL;

    // Read ID
    uuid_t neigh_id;
    ptr = YggEvent_readPayload(ev, ptr, neigh_id, sizeof(uuid_t));

    // Skip MAC Addr
    ptr += WLAN_ADDR_LEN;

    // Read LQs
    double rx_lq = 0.0, tx_lq = 0.0;
    ptr = YggEvent_readPayload(ev, ptr, &rx_lq, sizeof(double));
    ptr = YggEvent_readPayload(ev, ptr, &tx_lq, sizeof(double));

    // Read Traffic
    double traffic = 0.0;
    ptr = YggEvent_readPayload(ev, ptr, &traffic, sizeof(double));

    // Read Neighbor Type
    byte is_bi = false;
    ptr = YggEvent_readPayload(ev, ptr, &is_bi, sizeof(byte));

    graph_node* node = graph_find_node(neighborhood, neigh_id);
    // The node might alredy exist and not be my neighbor
    if( node == NULL ) {
        unsigned char* key = malloc(sizeof(uuid_t));
        uuid_copy(key, neigh_id);

        graph_insert_node(neighborhood, key, NULL);
    }

    graph_edge* edge = graph_find_edge(neighborhood, neigh_id, myID);
    assert(edge == NULL);

    double* label = malloc(sizeof(double));
    *label = rx_lq;
    graph_insert_edge(neighborhood, neigh_id, myID, label);

    if( is_bi ) {
        edge = graph_find_edge(neighborhood, myID, neigh_id);
        assert(edge == NULL);

        label = malloc(sizeof(double));
        *label = tx_lq;
        graph_insert_edge(neighborhood, myID, neigh_id, label);
    }

    // Read Neighbors
    byte n_neighs = 0;
    ptr = YggEvent_readPayload(ev, ptr, &n_neighs, sizeof(byte));
    for(int i = 0; i < n_neighs; i++) {
        // Read Neigh ID
        uuid_t two_hop_neigh_id;
        ptr = YggEvent_readPayload(ev, ptr, two_hop_neigh_id, sizeof(uuid_t));

        // Read Neigh LQ
        ptr = YggEvent_readPayload(ev, ptr, &rx_lq, sizeof(double));
        ptr = YggEvent_readPayload(ev, ptr, &tx_lq, sizeof(double));

        // Read Traffic
        ptr = YggEvent_readPayload(ev, ptr, &traffic, sizeof(double));

        // Read Neigh Type
        ptr = YggEvent_readPayload(ev, ptr, &is_bi, sizeof(byte));

        node = graph_find_node(neighborhood, two_hop_neigh_id);
        if( node == NULL ) {
            unsigned char* key = malloc(sizeof(uuid_t));
            uuid_copy(key, two_hop_neigh_id);

            graph_insert_node(neighborhood, key, NULL);
        }

        edge = graph_find_edge(neighborhood, two_hop_neigh_id, neigh_id);
        if(edge == NULL) {
            label = malloc(sizeof(double));
            *label = rx_lq;
            graph_insert_edge(neighborhood, two_hop_neigh_id, neigh_id, label);
        } else {
            *((double*)edge->label) = rx_lq;
        }

        if( is_bi ) {
            edge = graph_find_edge(neighborhood, neigh_id, two_hop_neigh_id);
            if(edge == NULL) {
                label = malloc(sizeof(double));
                *label = tx_lq;
                graph_insert_edge(neighborhood, neigh_id, two_hop_neigh_id, label);
            } else {
                *((double*)edge->label) = tx_lq;
            }
        }
    }
}

static void updateNeighbor(graph* neighborhood, unsigned char* myID, YggEvent* ev) {
    assert(ev && ev->payload && ev->length > 0);

    void* ptr = NULL;

    // Read ID
    uuid_t neigh_id;
    ptr = YggEvent_readPayload(ev, ptr, neigh_id, sizeof(uuid_t));

    // Skip MAC Addr
    ptr += WLAN_ADDR_LEN;

    // Read LQs
    double rx_lq = 0.0, tx_lq = 0.0;
    ptr = YggEvent_readPayload(ev, ptr, &rx_lq, sizeof(double));
    ptr = YggEvent_readPayload(ev, ptr, &tx_lq, sizeof(double));

    // Read Traffic
    double traffic = 0.0;
    ptr = YggEvent_readPayload(ev, ptr, &traffic, sizeof(double));

    // Read Neighbor Type
    byte is_bi = false;
    ptr = YggEvent_readPayload(ev, ptr, &is_bi, sizeof(byte));

    graph_node* node = graph_find_node(neighborhood, neigh_id);
    assert(node);

    graph_edge* edge = graph_find_edge(neighborhood, neigh_id, myID);
    assert(edge);
    *((double*)edge->label) = rx_lq;

    edge = graph_find_edge(neighborhood, myID, neigh_id);
    if( is_bi ) {
        if( edge ) {
            *((double*)edge->label) = tx_lq;
        } else {
            // Insert
            double* label = malloc(sizeof(double));
            *label = tx_lq;
            graph_insert_edge(neighborhood, myID, neigh_id, label);
        }
    } else {
        if( edge ) {
            // Remove
            void* label = graph_remove_edge(neighborhood, myID, neigh_id);
            free(label);
        }
    }

    // Read Neighbors
    byte n_neighs = 0;
    ptr = YggEvent_readPayload(ev, ptr, &n_neighs, sizeof(byte));
    for(int i = 0; i < n_neighs; i++) {
        // Read Neigh ID
        uuid_t two_hop_neigh_id;
        ptr = YggEvent_readPayload(ev, ptr, &two_hop_neigh_id, sizeof(uuid_t));

        // Read Neigh LQ
        ptr = YggEvent_readPayload(ev, ptr, &rx_lq, sizeof(double));
        ptr = YggEvent_readPayload(ev, ptr, &tx_lq, sizeof(double));

        // Read Traffic
        ptr = YggEvent_readPayload(ev, ptr, &traffic, sizeof(double));

        // Read Neigh Type
        ptr = YggEvent_readPayload(ev, ptr, &is_bi, sizeof(byte));

        node = graph_find_node(neighborhood, two_hop_neigh_id);
        if( !node ) {
            unsigned char* key = malloc(sizeof(uuid_t));
            uuid_copy(key, two_hop_neigh_id);

            graph_insert_node(neighborhood, key, NULL);
        }

        edge = graph_find_edge(neighborhood, two_hop_neigh_id, neigh_id);
        if( edge ) {
            *((double*)edge->label) = rx_lq;
        } else {
            double* label = malloc(sizeof(double));
            *label = rx_lq;
            graph_insert_edge(neighborhood, two_hop_neigh_id, neigh_id, label);
        }

        edge = graph_find_edge(neighborhood, neigh_id, two_hop_neigh_id);
        if( is_bi ) {
            if( edge ) {
                *((double*)edge->label) = tx_lq;
            } else {
                double* label = malloc(sizeof(double));
                *label = tx_lq;
                graph_insert_edge(neighborhood, neigh_id, two_hop_neigh_id, label);
            }
        } else {
            if( edge ) {
                // Remove
                void* label = graph_remove_edge(neighborhood, neigh_id, two_hop_neigh_id);
                free(label);
            }
        }
    }
}

static void removeNeighbor(graph* neighborhood, unsigned char* myID, YggEvent* ev) {
    assert(ev && ev->payload && ev->length > 0);

    void* ptr = NULL;

    // Read ID
    uuid_t neigh_id;
    ptr = YggEvent_readPayload(ev, ptr, &neigh_id, sizeof(uuid_t));

    // Read Neighbor Type
    byte is_bi = false;
    ptr = YggEvent_readPayload(ev, ptr, &is_bi, sizeof(byte));

    void* label = graph_remove_edge(neighborhood, neigh_id, myID);
    free(label);
    if( is_bi ) {
        label = graph_remove_edge(neighborhood, myID, neigh_id);
        free(label);
    }

    // Read Neighbors
    byte n_neighs = 0;
    ptr = YggEvent_readPayload(ev, ptr, &n_neighs, sizeof(byte));
    for(int i = 0; i < n_neighs; i++) {
        // Read Neigh ID
        uuid_t two_hop_neigh_id;
        ptr = YggEvent_readPayload(ev, ptr, &two_hop_neigh_id, sizeof(uuid_t));

        // Read Neigh Type
        ptr = YggEvent_readPayload(ev, ptr, &is_bi, sizeof(byte));

        void* label = graph_remove_edge(neighborhood, two_hop_neigh_id, neigh_id);
        free(label);
        if( is_bi ) {
            label = graph_remove_edge(neighborhood, neigh_id, two_hop_neigh_id);
            free(label);
        }

        graph_node* node = graph_find_node(neighborhood, two_hop_neigh_id);
        assert(node);
        if( node->in_adjacencies->size == 0 && node->out_adjacencies->size == 0 ) {
            void* value = graph_remove_node(neighborhood, two_hop_neigh_id);
            assert(value == NULL);
        }
    }

    void* value = graph_remove_node(neighborhood, neigh_id);
    assert(value == NULL);
}

*/


static graph* extractNeighborhood(YggEvent* ev) {
    assert(ev && ev->payload && ev->length > 0);

    void* ptr = NULL;

    graph* neighborhood = graph_init((key_comparator)&uuid_compare, sizeof(uuid_t));

    // Nodes
    byte n_nodes = 0;
    ptr = YggEvent_readPayload(ev, ptr, &n_nodes, sizeof(n_nodes));

    for(int i = 0; i < n_nodes; i++) {
        // node id
        unsigned char* node_id = malloc(sizeof(uuid_t));
        ptr = YggEvent_readPayload(ev, ptr, node_id, sizeof(uuid_t));

        // node mac addr (being ignored)
        byte node_addr[WLAN_ADDR_LEN] = {0};
        ptr = YggEvent_readPayload(ev, ptr, node_addr, WLAN_ADDR_LEN);

        // node out traffic
        double* node_out_traffic = malloc(sizeof(double));
        ptr = YggEvent_readPayload(ev, ptr, node_out_traffic, sizeof(double));

        graph_insert_node(neighborhood, node_id, node_out_traffic);
    }

    // Edges
    byte n_edges = 0;
    ptr = YggEvent_readPayload(ev, ptr, &n_edges, sizeof(n_edges));

    for(int i = 0; i < n_edges; i++) {
        // start node id
        uuid_t start_node_id = {0};
        ptr = YggEvent_readPayload(ev, ptr, start_node_id, sizeof(uuid_t));

        // end node id
        uuid_t end_node_id = {0};
        ptr = YggEvent_readPayload(ev, ptr, end_node_id, sizeof(uuid_t));

        // link quality
        double* lq = malloc(sizeof(double));
        ptr = YggEvent_readPayload(ev, ptr, lq, sizeof(double));

        graph_insert_edge(neighborhood, start_node_id, end_node_id, lq);
    }

    return neighborhood;
}

static void updateEnvironment(NeighborsContextState* state, YggEvent* ev) {
    assert(ev && ev->payload && ev->length > 0);

    void* ptr = NULL;

    ptr = YggEvent_readPayload(ev, ptr, &state->in_traffic, sizeof(double));

    ptr = YggEvent_readPayload(ev, ptr, &state->out_traffic, sizeof(double));

    ptr = YggEvent_readPayload(ev, ptr, &state->new_neighbors_flux, sizeof(double));

    ptr = YggEvent_readPayload(ev, ptr, &state->lost_neighbors_flux, sizeof(double));

    ptr += sizeof(unsigned int);

    ptr = YggEvent_readPayload(ev, ptr, &state->neighbors_density, sizeof(double));
}

static unsigned int minMaxNeighs(graph* neighborhood, unsigned char* myID, bool max, AdjacencyType adj_type) {

    list* adj = graph_get_adjacencies(neighborhood, myID, adj_type);
    assert(adj != NULL);

	unsigned int value = 0;
	for(list_item* it = adj->head; it; it = it->next) {
        int deg = graph_get_node_in_degree(neighborhood, it->data);
        assert(deg >= 0);

		if(max)
			value = iMax(value, deg);
		else
			value = iMin(value, deg);
	}

    list_delete(adj);

	return value;
}

double avgNeighs(graph* neighborhood, unsigned char* myID, bool include_me, AdjacencyType adj_type) {
    list* adj = graph_get_adjacencies(neighborhood, myID, adj_type);
    assert(adj != NULL);

	unsigned int sum = 0;
	for(list_item* it = adj->head; it; it = it->next) {
        int deg = graph_get_node_in_degree(neighborhood, it->data);
        assert(deg >= 0);

		sum += deg;
	}
    unsigned int total = adj->size;
    list_delete(adj);

    if(include_me) {
        sum += total;
        total++;
    }

    double avg = ((double)sum) / total;

	return avg;
}

static list* getneighborsInCommon(graph* neighborhood, unsigned char* neigh1_id, unsigned char* neigh2_id) {
	assert(neighborhood != NULL);

	list* result = list_init();

	graph_node* node1 = graph_find_node(neighborhood, neigh1_id);
	graph_node* node2 = graph_find_node(neighborhood, neigh2_id);

    if(node1 == NULL || node2 == NULL) {
        return result;
    }

    list* node1_neighs = graph_get_adjacencies_from_node(neighborhood, node1, SYM_ADJ);
    list* node2_neighs = graph_get_adjacencies_from_node(neighborhood, node2, SYM_ADJ);

    for(list_item* it1 = node1_neighs->head; it1; it1 = it1->next) {
		for(list_item* it2 = node2_neighs->head; it2; it2 = it2->next)  {
            if(uuid_compare(it1->data, it2->data) == 0) {
                unsigned char* id = malloc(sizeof(uuid_t));
				uuid_copy(id, it1->data);
				list_add_item_to_head(result, id);
            }
		}
	}

    list_delete(node1_neighs);
    list_delete(node2_neighs);

	return result;
}

static list* getCoverage(graph* neighborhood, unsigned char* myID, double_list* copies, bool onlyFirst) {
	list* total_coverage = list_init();

	for(double_list_item* it = copies->head; it; it = it->next) {
		MessageCopy* msg_copy = (MessageCopy*)it->data;

        unsigned char* parent = getBcastHeader(msg_copy)->sender_id;

        unsigned char* x = malloc(sizeof(uuid_t));
        uuid_copy(x, parent);
        list_add_item_to_tail(total_coverage, x);

		list* neigh_coverage = getneighborsInCommon(neighborhood, myID, parent);

		list_append(total_coverage, neigh_coverage);

        if(onlyFirst)
            break;
	}

	return total_coverage;
}

static list* notCovered(graph* neighborhood, unsigned char* myID, double_list* copies) {
	list* missed = list_init();

	list* total_coverage = getCoverage(neighborhood, myID, copies, false);

    list* adj = graph_get_adjacencies(neighborhood, myID, SYM_ADJ);

    for(list_item* it = adj->head; it; it = it->next) {
		unsigned char* neigh_id = (unsigned char*)it->data;

		unsigned char* aux = list_remove_item(total_coverage, (comparator_function)&equalID, neigh_id);
		if( aux != NULL ) {
			free(aux);
		} else {
			unsigned char* id = malloc(sizeof(uuid_t));
			uuid_copy(id, neigh_id);
			list_add_item_to_tail(missed, id);
		}
	}
	list_delete(total_coverage);

    list_delete(adj);

	return missed;
}

static bool allCovered(graph* neighborhood, unsigned char* myID, double_list* copies) {
	list* missed = notCovered(neighborhood, myID, copies);
	bool all_covered = missed->size == 0;
	list_delete(missed);
	return all_covered;
}

static void getneighborsDistribution(graph* neighborhood, unsigned char* myID, double* result) {
	unsigned int max = minMaxNeighs(neighborhood, myID, true, IN_ADJ);
	graph_node* node = graph_find_node(neighborhood, myID);
	assert(node != NULL);
	unsigned int n = node->in_adjacencies->size;

	result[0] = n;
	result[1] = 0.0;
	result[2] = 0.0;

	if( max > 0 && n > 0 ) {
		int* dist = malloc((max+1)*sizeof(int));
		bzero(dist, (max+1)*sizeof(int));

		int nn = n, sum = n;
		dist[nn]++;

        for(list_item* it = node->in_adjacencies->head; it; it = it->next) {
            graph_edge* edge = (graph_edge*)it->data;
			nn = edge->start_node->in_adjacencies->size;
			dist[nn]++;
			sum += nn;
		}

		if( sum > 0 && n > 0 ) {
			double p=0.0, cdf=0.0;
			for(int i = max-1; i >= n; i--) {
				p =  ((double) dist[i]) / sum;
				cdf = p + cdf;
			}
			result[1] = p;
			result[2] = cdf;
		}
		free(dist);
	}
}
