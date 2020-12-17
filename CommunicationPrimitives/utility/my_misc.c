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

#include "my_misc.h"

#include <uuid/uuid.h>
#include <stdlib.h>

#include "Yggdrasil/core/utils/hashfunctions.h"

#include "utility/my_sys.h"
#include <linux/limits.h>

#include <assert.h>

Tuple* newTuple(void** entries, unsigned int size) {
    Tuple* tuple = malloc(sizeof(Tuple));
    tuple->size = size;
    tuple->entries = malloc(size*sizeof(void*));
    for(int i = 0; i < size; i++) {
        tuple->entries[i] = entries[i];
    }
    return tuple;
}

bool equalID(void* a, void* b) {
	return uuid_compare((unsigned char*)a, (unsigned char*)b) == 0;
}

unsigned long uuid_hash(unsigned char* id) {
	return RSHash(id, sizeof(uuid_t));
}

bool equalInt(void* a, void* b) {
	return *((int*)a) == *((int*)b);
}

unsigned long int_hash(int* n) {
	return RSHash(n, sizeof(int));
}

/* Function to sort an array using insertion sort
 * Adapted from: https://www.geeksforgeeks.org/insertion-sort/
 * */
void insertionSort(void* v, unsigned int element_size, unsigned int n_elements, int (*cmp)(void*,void*)) {
	int i, j;
	unsigned char key[element_size];
	for (i = 1; i < n_elements; i++) {
		memcpy(key, v + i*element_size, element_size);
		j = i - 1;

		/* Move elements of v[0..i-1], that are
		greater than key, to one position ahead
		of their current position */
		while (j >= 0 && cmp(v + j*element_size, key) > 0) {
			memcpy(v + (j+1)*element_size, v + j*element_size, element_size);
			j = j - 1;
		}

		memcpy(v + (j+1)*element_size, key, element_size);
	}
}

void pushMessageType(YggMessage* msg, unsigned char type) {
    unsigned short type_len = sizeof(type);
	char data[YGG_MESSAGE_PAYLOAD];

	memcpy(data, &type, type_len);
	memcpy(data + type_len, msg->data, msg->dataLen);

	memcpy(msg->data, data, msg->dataLen + type_len);
	msg->dataLen = msg->dataLen + type_len;
}

unsigned char popMessageType(YggMessage* msg) {
    unsigned short type_len = sizeof(unsigned char);
	unsigned char type = 0;
	YggMessage_readPayload(msg, NULL, &type, type_len);

	unsigned short dataLen = msg->dataLen - type_len;
	char data[YGG_MESSAGE_PAYLOAD];

	memcpy(data, msg->data + type_len, dataLen);

	memcpy(msg->data, data, dataLen);
	msg->dataLen = dataLen;

	return type;
}


/*
list* get_bidirectional_stable_neighbors(graph* neighborhood, unsigned char* id) {
    list* neighbors = list_init();

    // printf("GET BI NEIGHS:\n");
    // char neigh_str[UUID_STR_LEN+1];
    // neigh_str[UUID_STR_LEN] = '\0';
    // char neigh2_str[UUID_STR_LEN+1];
    // neigh2_str[UUID_STR_LEN] = '\0';


    graph_node* node = graph_find_node(neighborhood, id);
    //if(node != NULL) {
        for(list_item* it = node->in_adjacencies->head; it; it = it->next) {
            graph_edge* edge = it->data;
            edge_label* ex = (edge_label*)edge->label;

            //uuid_unparse(edge->start_node->key, neigh_str);
            //uuid_unparse(edge->end_node->key, neigh2_str);
            //printf("   %s-->%s : %s\n", neigh_str, neigh2_str, ex->pending?"true":"false");

            if(!ex->pending) {
                // If the inverse edge exists
                graph_edge* edge2 = graph_find_edge(neighborhood, edge->end_node->key, edge->start_node->key);
                if(edge2 != NULL) {
                    edge_label* ex2 = (edge_label*)edge2->label;

                    //uuid_unparse(edge2->start_node->key, neigh_str);
                    //uuid_unparse(edge2->end_node->key, neigh2_str);
                    //printf("      %s-->%s : %s\n", neigh_str, neigh2_str, ex2->pending?"true":"false");

                    if(!ex2->pending) {
                        unsigned char* neigh_id = malloc(sizeof(uuid_t));
                        uuid_copy(neigh_id, edge->start_node->key);
                        list_add_item_to_tail(neighbors, neigh_id);
                    }
                }
            }
        }
    //}

    return neighbors;
}

*/


/*
list* compute_mprs(graph* neighborhood, unsigned char* myID) {

    list* mprs = list_init();

    if(neighborhood->nodes->size == 0) {
        return mprs;
    }

    list* neighbors = get_bidirectional_stable_neighbors(neighborhood, myID);
    list* neighbors_2 = list_init();
    list* aux = list_init();

    void* del = NULL;

    //printf("bi-neighbors: %d\n", neighbors->size);

    char neigh_str[UUID_STR_LEN+1];
    neigh_str[UUID_STR_LEN] = '\0';

    // For each neighbors, verify if of all its bidirectional neighbors, there exists one that has only one bidirectional neighbor
    for(list_item* it = neighbors->head; it; it = it->next) {
        unsigned char* neigh_id = it->data;
        list* n_neighs = get_bidirectional_stable_neighbors(neighborhood, neigh_id);
        del = list_remove_item(n_neighs, &equalID, myID);
        if(del != NULL) free(del);

        uuid_unparse(neigh_id, neigh_str);
        //printf("   neigh: %s\n", neigh_str);

        bool inserted = false;

        for(list_item* it2 = n_neighs->head; it2; it2 = it2->next) {

            unsigned char* neigh2_id = it2->data;
            list* nn_neighs = get_bidirectional_stable_neighbors(neighborhood, neigh2_id);
            del = list_remove_item(nn_neighs, &equalID, neigh_id);
            if(del != NULL) free(del);

            uuid_unparse(neigh2_id, neigh_str);
            //printf("      neigh2: %s\n", neigh_str);

            if(nn_neighs->size == 0) {
                if(list_find_item(mprs, &equalID, neigh_id) == NULL) {
                    unsigned char* x = malloc(sizeof(uuid_t));
                    uuid_copy(x, neigh_id);
                    list_add_item_to_tail(mprs, x);
                    inserted = true;
                    //printf("   neigh inserted");
                }
            }

            // Remove to avoid duplicates in the future
            del = list_remove_item(neighbors_2, &equalID, neigh2_id);
            if(del != NULL) free(del);

            list_delete(nn_neighs);
        }

        if(!inserted) {
            unsigned char* x = malloc(sizeof(uuid_t));
            uuid_copy(x, neigh_id);
            list_add_item_to_tail(aux, x);
        }

        // Apped the lists. no duplicates are present
        list_append(neighbors_2, n_neighs);
    }

    //void* del = list_remove_item(neighbors_2, &equalID, myID);
    //if(del != NULL) free(del);

    // remove neighbors from neighbors_2
    for(list_item* it = neighbors->head; it; it = it->next) {
        unsigned char* neigh_id = it->data;

        del = list_remove_item(neighbors_2, &equalID, neigh_id);
        if(del != NULL) free(del);
    }

    // Remove from neighbors_2, the mprs' neighbors
    for(list_item* it = mprs->head; it; it = it->next) {
        unsigned char* id = it->data;
        list* neighs = get_bidirectional_stable_neighbors(neighborhood, id);

        for(list_item* it2 = neighs->head; it2; it2 = it2->next) {
            unsigned char* id2 = it2->data;
            del = list_remove_item(neighbors_2, &equalID, id2);
            if(del != NULL) free(del);
        }

        list_delete(neighs);
    }

    // set neighbors as the set of neighors which are not mprs
    list_delete(neighbors);
    neighbors = aux;
    aux = NULL;

    //
    while(!list_is_empty(neighbors_2)) {
        assert(neighbors != NULL && neighbors->size > 0);

        // Get Neighbor with the maximum 2 hop (from the current node) uncovered neighbors
        unsigned int max_uncovered_neighs = 0;
        uuid_t max_neigh_id;
        memset(max_neigh_id, 0, sizeof(uuid_t));
        list* max_neigh_neighs = NULL;

        for(list_item* it = neighbors->head; it; it = it->next) {
            unsigned char* neigh_id = it->data;

            uuid_unparse(neigh_id, neigh_str);
            //printf("   neigh: %s\n", neigh_str);

            list* neighs = get_bidirectional_stable_neighbors(neighborhood, neigh_id);
            //del = list_remove_item(neighs, &equalID, myID);
            //0if(del != NULL) free(del);

            unsigned int uncovered_neighs = 0;

            for(list_item* it2 = neighs->head; it2; it2 = it2->next) {
                unsigned char* neigh2_id = it2->data;
                if(list_find_item(neighbors_2, &equalID, neigh2_id)) {
                    uncovered_neighs++;
                }
            }

            if(uncovered_neighs > max_uncovered_neighs) {
                max_uncovered_neighs = uncovered_neighs;
                uuid_copy(max_neigh_id, neigh_id);

                if(max_neigh_neighs != NULL) {
                    list_delete(max_neigh_neighs);
                }
                max_neigh_neighs = neighs;
            } else {
                list_delete(neighs);
            }
        }
        assert(max_neigh_neighs != NULL);

        if(max_neigh_neighs != NULL) {
            // Add to mprs
            unsigned char* id = malloc(sizeof(uuid_t));
            uuid_copy(id, max_neigh_id);
            list_add_item_to_tail(mprs, id);

            uuid_unparse(id, neigh_str);
            //printf("   mpr: %s\n", neigh_str);

            // Update neighbors and neighbors_2
            del = list_remove_item(neighbors, &equalID, max_neigh_id);
            assert(del != NULL);
            if(del != NULL)
                free(del);

            for(list_item* it = max_neigh_neighs->head; it; it = it->next) {
                unsigned char* neigh_id = it->data;

                del = list_remove_item(neighbors_2, &equalID, neigh_id);
                if(del != NULL) free(del);
            }
        }
    }

    list_delete(neighbors);
    list_delete(neighbors_2);

    return mprs;
}
*/



int is_memory_zero(const void* addr, unsigned long size) {

    unsigned char* ptr = (unsigned char*)addr;
    unsigned int remaining = size % sizeof(long);

    int not_zero = 0;

    for(unsigned int i = 0; i < remaining; i++) {
        not_zero |= *(ptr+i);
    }

    for (unsigned long i = remaining; i < size; i += sizeof(long) ) {
        not_zero |= *(long*)(ptr+i);
    }

    return !not_zero;
}


topology_manager_args* load_overlay(char* overlay_path, char* hostname) {
    // Register Topology Manager Protocol
	char db_file_path[PATH_MAX];
	char neighs_file_path[PATH_MAX];
	build_path(db_file_path, overlay_path, "macAddrDB.txt");
	//build_path(neighs_file_path, config.app.overlay_path, "neighs.txt");

    char neighs_file[20];
    sprintf(neighs_file, "%s.txt", hostname);
    build_path(neighs_file_path, overlay_path, neighs_file);

    char str[1000];
    char cmd[PATH_MAX + 300];
    sprintf(cmd, "cat %s | wc -l", db_file_path);
    int n = run_command(cmd, str, 1000);
    assert(n > 0);

    int db_size = (int) strtol(str, NULL, 10);

	topology_manager_args* t_args = topology_manager_args_init(db_size, db_file_path, neighs_file_path, true);
	return t_args;
}

bool equalAddr(void* a, void* b) {
	return *((void**)a) == b;
}

bool is_unicast_message(YggMessage* msg) {
    WLANAddr* bcast_addr = getBroadcastAddr();
    WLANAddr* addr = &msg->destAddr;
    bool is_unicast_addr = memcmp(addr->data, bcast_addr->data, WLAN_ADDR_LEN) != 0;
    free(bcast_addr);

    return is_unicast_addr;
}
