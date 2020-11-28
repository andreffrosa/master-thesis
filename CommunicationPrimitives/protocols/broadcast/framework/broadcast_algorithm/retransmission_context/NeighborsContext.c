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

#include "protocols/discovery/topology_discovery.h"
#include "data_structures/graph.h"
#include "data_structures/list.h"
#include "utility/my_misc.h"
#include "utility/my_math.h"
#include "utility/my_time.h"

#include <assert.h>

typedef struct _NeighborsContextState {
	graph* neighborhood;
    list* stability;
    list* in_traffic;
    list* out_traffic;
    list* misses;
    unsigned int window_size;
} NeighborsContextState;

typedef struct _NeighborsContextArgs {
	topology_discovery_args* d_args;
	//bool append_neighbors;
} NeighborsContextArgs;

static unsigned int minMaxNeighs(graph* neighborhood, unsigned char* myID, bool max, AdjacencyType adj_type);
static double avgNeighs(graph* neighborhood, unsigned char* myID, bool include_me, AdjacencyType adj_type);
static void getNeighboursDistribution(graph* neighborhood, unsigned char* myID, double* result);
static list* getNeighboursInCommon(graph* neighborhood, uuid_t neigh1_id, uuid_t neigh2_id);
static list* getCoverage(graph* neighborhood, unsigned char* myID, double_list* copies, bool onlyFirst);
static list* notCovered(graph* neighborhood, unsigned char* myID, double_list* copies);
static bool allCovered(graph* neighborhood, unsigned char* myID, double_list* copies);


static void windowGC(list* window, unsigned int window_size, struct timespec* current_time);
static void updateWindow(list* window, unsigned int window_size, struct timespec* current_time);
/*static*/ double getWindow(list* window, unsigned int window_size, struct timespec* current_time, char* type);

static void process_node(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id, unsigned char n_neighs, unsigned long version) {
	graph* neighborhood = (graph *)state;

	unsigned char* _id = malloc(sizeof(uuid_t));
	uuid_copy(_id, id);

	graph_insert_node(neighborhood, _id, NULL);
}

static void process_edge(void* state, unsigned int current_level, bool is_last_level, int index, unsigned char* id1, unsigned long version, unsigned char* id2, double cost) {
	graph* neighborhood = (graph*)state;

    double* c = malloc(sizeof(double));
    *c = cost;

	graph_insert_edge(neighborhood, id2, id1, c);
}

static void createGraph(NeighborsContextState* state, void* announce, int size, unsigned char* myID) {

	graph_delete(state->neighborhood);

	state->neighborhood = graph_init_complete((key_comparator)&uuid_compare, NULL, NULL, sizeof(uuid_t), 0, sizeof(double));

    unsigned char* my_id = malloc(sizeof(uuid_t));
    uuid_copy(my_id, myID);
	graph_insert_node(state->neighborhood, my_id, NULL);

    // DEBUG
    /*char* str;
    printAnnounce(announce, size, 2, &str);
    printf("%s\n%s", "Serialized Announce", str);
    free(str);*/

	processAnnounce(state->neighborhood, announce, size, -1, &process_node, &process_edge);
}

static void NeighborsContextInit(ModuleState* context_state, proto_def* protocol_definition, unsigned char* myID, list* visited) {
	// Register Topology Discovery Protocol
	registerProtocol(TOPOLOGY_DISCOVERY_PROTO_ID, &topology_discovery_init, ((NeighborsContextArgs*)(context_state->args))->d_args);
	proto_def_add_consumed_event(protocol_definition, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBOR_FOUND);
	proto_def_add_consumed_event(protocol_definition, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBOR_UPDATE);
	proto_def_add_consumed_event(protocol_definition, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBOR_LOST);
    proto_def_add_consumed_event(protocol_definition, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBORHOOD_UPDATE);
    proto_def_add_consumed_event(protocol_definition, TOPOLOGY_DISCOVERY_PROTO_ID, IN_TRAFFIC);
    proto_def_add_consumed_event(protocol_definition, TOPOLOGY_DISCOVERY_PROTO_ID, OUT_TRAFFIC);

    NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);
    if(state->neighborhood == NULL) {
        state->neighborhood = graph_init((key_comparator)&uuid_compare, sizeof(uuid_t));
        unsigned char* my_id = malloc(sizeof(uuid_t));
        uuid_copy(my_id, myID);
        graph_insert_node(state->neighborhood, my_id, NULL);
    }
}

static unsigned int NeighborsContextHeader(ModuleState* context_state, PendingMessage* p_msg, void** context_header, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	/*NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);
	NeighborsContextArgs* args = (NeighborsContextArgs*) (context_state->args);

	if(args->append_neighbors) {
		unsigned int size = sizeof(unsigned char);
		*context_header = malloc(size);

		int deg = graph_get_node_in_degree(state->neighborhood, myID);
		assert(deg >= 0);

		*((unsigned char*)*context_header) = deg;
		return size;
	}*/

	*context_header = NULL;
	return 0;
}

static void NeighborsContextEvent(ModuleState* context_state, queue_t_elem* elem, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    if(elem->type == YGG_EVENT) {
		if(elem->data.event.notification_id == NEIGHBOR_FOUND) {
            updateWindow(state->stability, state->window_size, &current_time);
		} else if(elem->data.event.notification_id == NEIGHBOR_UPDATE) {
			// Nothing
		} else if(elem->data.event.notification_id == NEIGHBOR_LOST) {
			updateWindow(state->stability, state->window_size, &current_time);
		} else if(elem->data.event.notification_id == NEIGHBORHOOD_UPDATE) {
			createGraph((NeighborsContextState*)(context_state->vars), elem->data.event.payload, elem->data.event.length, myID);
		} else if(elem->data.event.notification_id == IN_TRAFFIC) {
            updateWindow(state->in_traffic, state->window_size, &current_time);

            unsigned int misses = *(unsigned int*)elem->data.event.payload;
            for(int i = 0; i < misses; i++) {
                updateWindow(state->misses, state->window_size, &current_time);
            }
        } else if(elem->data.event.notification_id == OUT_TRAFFIC) {
            updateWindow(state->out_traffic, state->window_size, &current_time);
        }
	}
}

static bool NeighborsContextQuery(ModuleState* context_state, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
	NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);

	if(strcmp(query, "graph") == 0 || strcmp(query, "neighborhood") == 0) {
		*((graph**)result) = graph_clone(state->neighborhood);
		return true;
	} else if(strcmp(query, "n_neighbors") == 0 || strcmp(query, "degree") == 0 ) {
        unsigned char* id = argc >= 1 ? va_arg(*argv, unsigned char*) : myID;

        list* sym_neighs = graph_get_adjacencies(state->neighborhood, id, SYM_ADJ);
        int deg = sym_neighs->size;
        list_delete(sym_neighs);

        *((unsigned int*)result) = deg;
		return true;
	} else if(strcmp(query, "in_degree") == 0) {
        unsigned char* id = argc >= 1 ? va_arg(*argv, unsigned char*) : myID;

		int deg = graph_get_node_in_degree(state->neighborhood, id);
		assert(deg >= 0);

        *((unsigned int*)result) = deg;
		return true;
	} else if(strcmp(query, "out_degree") == 0) {
        unsigned char* id = argc >= 1 ? va_arg(*argv, unsigned char*) : myID;

		int deg = graph_get_node_out_degree(state->neighborhood, id);
		assert(deg >= 0);

        *((unsigned int*)result) = deg;
		return true;
	} else if(strcmp(query, "in_neighbors") == 0 ) {
        //assert(argc >= 1);
        unsigned char* id = argc >= 1 ? va_arg(*argv, unsigned char*) : myID;
        *((list**)result) = graph_get_adjacencies(state->neighborhood, id, IN_ADJ);
		return true;
	} else if(strcmp(query, "out_neighbors") == 0 ) {
        //assert(argc >= 1);
        unsigned char* id = argc >= 1 ? va_arg(*argv, unsigned char*) : myID;
        *((list**)result) = graph_get_adjacencies(state->neighborhood, id, OUT_ADJ);
		return true;
	} else if(strcmp(query, "neighbors") == 0 || strcmp(query, "sym_neighbors") == 0 ) {
        //assert(argc >= 1);
        unsigned char* id = argc >= 1 ? va_arg(*argv, unsigned char*) : myID;
        *((list**)result) = graph_get_adjacencies(state->neighborhood, id, SYM_ADJ);
		return true;
	} else if(strcmp(query, "max_neighbors") == 0) {
        AdjacencyType adj_type = IN_ADJ;
        if(argc >= 1) {
            adj_type = va_arg(*argv, AdjacencyType);
        }
		*((unsigned int*)result) = minMaxNeighs(state->neighborhood, myID, true, adj_type);
		return true;
	} else if(strcmp(query, "min_neighbors") == 0) {
        AdjacencyType adj_type = IN_ADJ;
        if(argc >= 1) {
            adj_type = va_arg(*argv, AdjacencyType);
        }
		*((unsigned int*)result) = minMaxNeighs(state->neighborhood, myID, false, adj_type);
		return true;
	} else if(strcmp(query, "avg_neighbors") == 0) {
        assert(argc >= 1);
		bool include_me = va_arg(*argv, bool);
		*((double*)result) = avgNeighs(state->neighborhood, myID, include_me, IN_ADJ);
		return true;
	} else if(strcmp(query, "neighbors_distribution") == 0) {
		getNeighboursDistribution(state->neighborhood, myID, (double*)result);
		return true;
	} else if(strcmp(query, "coverage") == 0) {
		assert(argc >= 1);
		PendingMessage* p_msg = va_arg(*argv, PendingMessage*);

        if(argc >= 2) {
            char* arg1 = va_arg(*argv, char*);

            if(strcmp(arg1, "first")==0) {
                *((list**)result) = getCoverage(state->neighborhood, myID, getCopies(p_msg), true);
            } else if(strcmp(arg1, "all")==0) {
                *((list**)result) = getCoverage(state->neighborhood, myID, getCopies(p_msg), false);
            } else
                return false;
        } else {
            *((list**)result) = getCoverage(state->neighborhood, myID, getCopies(p_msg), false);
        }
		return true;
	} else if(strcmp(query, "not_covered") == 0) {
		assert(argc >= 1);
		PendingMessage* p_msg = va_arg(*argv, PendingMessage*);
		*((list**)result) = notCovered(state->neighborhood, myID, getCopies(p_msg));
		return true;
	} else if(strcmp(query, "all_covered") == 0) {
		assert(argc >= 1);
		PendingMessage* p_msg = va_arg(*argv, PendingMessage*);
		*((bool*)result) = allCovered(state->neighborhood, myID, getCopies(p_msg));
		return true;
	} else if(strcmp(query, "stability") == 0) {
        assert(argc >= 2);
        struct timespec* current_time = va_arg(*argv, struct timespec*);
        char* type = va_arg(*argv, char*);

        *((double*)result) = getWindow(state->stability, state->window_size, current_time, type);
		return true;
    } else if(strcmp(query, "in_traffic") == 0) {
        assert(argc >= 2);
        struct timespec* current_time = va_arg(*argv, struct timespec*);
        char* type = va_arg(*argv, char*);

        *((double*)result) = getWindow(state->in_traffic, state->window_size, current_time, type);
		return true;
    } else if(strcmp(query, "out_traffic") == 0) {
        assert(argc >= 2);
        struct timespec* current_time = va_arg(*argv, struct timespec*);
        char* type = va_arg(*argv, char*);

        *((double*)result) = getWindow(state->out_traffic, state->window_size, current_time, type);
		return true;
    } else if(strcmp(query, "misses") == 0) {
        assert(argc >= 2);
        struct timespec* current_time = va_arg(*argv, struct timespec*);
        char* type = va_arg(*argv, char*);

        *((double*)result) = getWindow(state->misses, state->window_size, current_time, type);
		return true;
    } else {
		return false;
	}
}

static bool NeighborsContextQueryHeader(ModuleState* context_state, void* context_header, unsigned int context_header_size, char* query, void* result, int argc, va_list* argv, RetransmissionContext* r_context, unsigned char* myID, list* visited) {
    /*NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);
    NeighborsContextArgs* args = (NeighborsContextArgs*) (context_state->args);

    if(args->append_neighbors) {
    	if(strcmp(query, "neighbors") == 0) {
    		*((unsigned int*)result) = context_header ? *((unsigned int*)context_header) : graph_get_node_in_degree(state->neighborhood, myID);

    		return true;
    	}
    }*/

	return false;
}

static void NeighborsContextDestroy(ModuleState* context_state, list* visited) {
    NeighborsContextArgs* args = context_state->args;
    destroy_topology_discovery_args(args->d_args);
    free(args);

    NeighborsContextState* state = (NeighborsContextState*) (context_state->vars);
    list_delete(state->stability);
    list_delete(state->in_traffic);
    list_delete(state->out_traffic);
    list_delete(state->misses);
    free(state);
}

RetransmissionContext* NeighborsContext(unsigned int window_size, topology_discovery_args* d_args/*, bool append_neighbors*/) {
	RetransmissionContext* r_context = malloc(sizeof(RetransmissionContext));

	NeighborsContextArgs* args = malloc(sizeof(NeighborsContextArgs));
	args->d_args = d_args;
	//args->append_neighbors = append_neighbors;

	r_context->context_state.args = args;

    NeighborsContextState* state = malloc(sizeof(NeighborsContextState));

    state->neighborhood = NULL;//graph_init((key_comparator)&uuid_compare, sizeof(uuid_t));
    //unsigned char* my_id = malloc(sizeof(uuid_t));
    //uuid_copy(my_id, state->myID);
    //graph_insert_node(state->neighborhood, my_id, NULL);
    state->stability = list_init();
    state->in_traffic = list_init();
    state->out_traffic = list_init();
    state->misses = list_init();
    state->window_size = window_size;

	r_context->context_state.vars = state;

	r_context->init = &NeighborsContextInit;
	r_context->create_header = &NeighborsContextHeader;
	r_context->process_event = &NeighborsContextEvent;
	r_context->query_handler = &NeighborsContextQuery;
	r_context->query_header_handler = &NeighborsContextQueryHeader;
    r_context->copy_handler = NULL;
    r_context->destroy = &NeighborsContextDestroy;

	return r_context;
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

static list* getNeighboursInCommon(graph* neighborhood, unsigned char* neigh1_id, unsigned char* neigh2_id) {
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
		message_copy* msg_copy = (message_copy*)it->data;

        unsigned char* parent = getBcastHeader(msg_copy)->sender_id;

        unsigned char* x = malloc(sizeof(uuid_t));
        uuid_copy(x, parent);
        list_add_item_to_tail(total_coverage, x);

		list* neigh_coverage = getNeighboursInCommon(neighborhood, myID, parent);

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

static void getNeighboursDistribution(graph* neighborhood, unsigned char* myID, double* result) {
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


static void windowGC(list* window, unsigned int window_size, struct timespec* window_start) {

    while (window->head != NULL && compare_timespec((struct timespec*)window->head->data, window_start) < 0) {
        void* x = list_remove_head(window);
        free(x);
    }
}

static void updateWindow(list* window, unsigned int window_size, struct timespec* current_time) {
    struct timespec window_duration, window_start;
    milli_to_timespec(&window_duration, window_size*1000);
    subtract_timespec(&window_start, current_time, &window_duration);

    windowGC(window, window_size, &window_start);

    void* x = malloc(sizeof(struct timespec));
    memcpy(x, current_time, sizeof(struct timespec));
    list_add_item_to_tail(window, x);
}

/*static*/ double getWindow(list* window, unsigned int window_size, struct timespec* current_time, char* type) {
     assert(type != NULL);

     struct timespec window_duration, window_start;
     milli_to_timespec(&window_duration, window_size*1000);
     subtract_timespec(&window_start, current_time, &window_duration);

     windowGC(window, window_size, &window_start);

     if(window->size == 0) {
         return 0.0;
     }

     char aux[strlen(type)+1];
     strcpy(aux, type);
     char* ptr = NULL;
     char* token  = strtok_r(aux, " ", &ptr);

     if(strcmp(token, "avg") == 0) {
         double avg = window->size / ((double)window_size); // changes per second
         //printf("avg: %f\n", avg);
         return avg;
     } else {
         unsigned int buckets[window_size];
         memset(buckets, 0, sizeof(buckets));
         for(list_item* it = window->head; it; it = it->next) {
             struct timespec* x = (struct timespec*)it->data;
             struct timespec aux;
             subtract_timespec(&aux, x, &window_start);
             unsigned int bucket = (unsigned int)(timespec_to_milli(&aux) / 1000); // integer division
             bucket = bucket==window_size ? bucket-1 : bucket;
             assert(bucket < window_size);
             buckets[bucket]++;
         }

         if(strcmp(token, "ema") == 0) {
             token = strtok_r(NULL, " ", &ptr);

             if(token != NULL) {
                 // Exponetial Moving Average
                 double alfa = strtod(token, NULL);
                 assert(0.0 <= alfa && alfa <= 1.0);

                 //printf("%d : %d\n", 0, buckets[0]);
                 double ema = buckets[0];
                 for(int i = 1; i < window_size; i++) {
                     ema = alfa*buckets[i] + (1-alfa)*ema;
                     //printf("%d : %d\n", i, buckets[i]);
                 }
                 //printf("ema = %f\n", ema);
                 return ema;
             } else {
     			printf("Parameter 1 of %s not passed!\n", "ema");
     			exit(-1);
     		}
        } else if(strcmp(token, "wma") == 0) {
            //printf("%d : %d\n", 0, buckets[0]);
            double wma = buckets[0];
            for(int i = 1; i < window_size; i++) {
                wma = (i+1)*buckets[i] + wma;
                //printf("%d : %d\n", i, buckets[i]);
            }
            wma /= (window_size*(window_size+1))/2.0;

            //printf("wma = %f\n", wma);
            return wma;
        }  else {
            printf("Unknown window type!\n");
            exit(-1);
        }
    }
}
