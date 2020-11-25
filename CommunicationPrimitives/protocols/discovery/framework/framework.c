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

#include "framework.h"
#include "handlers.h"

#include "utility/my_sys.h"

#include <assert.h>

void* discovery_framework_main_loop(main_loop_args* args);

static bool processTimer(discovery_framework_state* f_state, YggTimer* timer);
static bool processMessage(discovery_framework_state* f_state, YggMessage* message);
static bool processRequest(discovery_framework_state* f_state, YggRequest* request);
static bool processEvent(discovery_framework_state* f_state, YggEvent* event);

proto_def* discovery_framework_init(void* arg) {

	discovery_framework_state* f_state = malloc(sizeof(discovery_framework_state));
	f_state->args = (discovery_framework_args*)arg;

	proto_def* framework = create_protocol_definition(DISCOVERY_FRAMEWORK_PROTO_ID, DISCOVERY_FRAMEWORK_PROTO_NAME, f_state, NULL);
	proto_def_add_protocol_main_loop(framework, &discovery_framework_main_loop);
    proto_def_add_produced_events(framework, (DiscoveryEventType)DISCOVERY_EVENT_COUNT);

    f_state->dispatcher_queue = interceptProtocolQueue(PROTO_DISPATCH, DISCOVERY_FRAMEWORK_PROTO_ID);

    clock_gettime(CLOCK_MONOTONIC, &f_state->current_time); // Update current time

	// Initialize f_state variables
	DF_init(f_state);

	return framework;
}

void* discovery_framework_main_loop(main_loop_args* args) {

	discovery_framework_state* f_state = args->state;

	queue_t_elem elem;
	while (1) {
		// Retrieve an event from the queue
		queue_pop(args->inBox, &elem);

        bool processed = false;

        // Update current time
		clock_gettime(CLOCK_MONOTONIC, &f_state->current_time);

		switch (elem.type) {
    		case YGG_TIMER:
                ; YggTimer* timer = &elem.data.timer;
                if(timer->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID) {
                    processed = processTimer(f_state, timer);
                } else {
                    queue_push(f_state->dispatcher_queue, &elem);
                    processed = true;
                }
    			break;

    		case YGG_MESSAGE:
                ; YggMessage* msg = &elem.data.msg;
                processed = processMessage(f_state, msg);
                break;
    		case YGG_REQUEST:
                ; YggRequest* req = &elem.data.request;
                if(req->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID) {
                    processed = processRequest(f_state, &elem.data.request);
                } else {
                    queue_push(f_state->dispatcher_queue, &elem);
                    processed = true;
                }
    			break;
    		case YGG_EVENT:
                ; YggEvent* event = &elem.data.event;
                if(event->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID) {
                    processEvent(f_state, &elem.data.event);
                    processed = false; // Events should always be processed by both the framework and the context(?)
                } else {
                    queue_push(f_state->dispatcher_queue, &elem);
                    processed = true;
                }
    			break;
    		default:
    			; // On purpose
    			char s[100];
    			sprintf(s, "Got weird queue elem, type = %u", elem.type);
    			ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", s);
    			exit(-1);
		}

        if(!processed) {
            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "Event not processed");
        }

		// Release memory of elem payload
		free_elem_payload(&elem);
	}

	return NULL;
}

static bool processTimer(discovery_framework_state* f_state, YggTimer* timer) {

    // HELLO Timer
    if( timer->timer_type == HELLO_TIMER ) {
        DF_uponHelloTimer(f_state, true, false);
        return true;
    }
    // HACK Timer
    else if( timer->timer_type == HACK_TIMER ) {
        DF_uponHackTimer(f_state, true);
        return true;
    }
    // Reply Timer
    else if( timer->timer_type == REPLY_TIMER ) {
        DF_uponReplyTimer(f_state, timer->payload, timer->length);
        return true;
    }
    // Neighbor Change Timer
    else if( timer->timer_type == NEIGHBOR_CHANGE_TIMER ) {
        DF_uponNeighborChangeTimer(f_state);
        return true;
    }
    // Neighbor Timer
    else if( timer->timer_type == NEIGHBOR_TIMER ) {
        NeighborEntry* neigh = NT_getNeighbor(f_state->neighbors, timer->id);
        assert(neigh);
        DF_uponNeighborTimer(f_state, neigh);
        return true;
    }
    // Discovery Environment Timer
    else if( timer->timer_type == DISCOVERY_ENVIRONMENT_TIMER ) {
        DF_uponDiscoveryEnvironmentTimer(f_state);
        return true;
    }

    return false;
}

static bool processMessage(discovery_framework_state* f_state, YggMessage* msg) {
    bool processed = false;

    // UpStream message
    if(msg->Proto_id == DISCOVERY_FRAMEWORK_PROTO_ID) {
        // Check if there are piggybacked discovery messages
        unsigned short piggyback_size = 0;
        memcpy(&piggyback_size, msg->data + sizeof(unsigned short), sizeof(piggyback_size));

        byte buffer[sizeof(piggyback_size) + piggyback_size];
        popPayload(msg, (char*)buffer, sizeof(piggyback_size) + piggyback_size);

        assert( piggyback_size == *((unsigned char*)buffer) );

        // Has discovery message
        if( piggyback_size > 0 ) {
            bool piggybacked = msg->Proto_id != DISCOVERY_FRAMEWORK_PROTO_ID;

            DF_processMessage(f_state, buffer + sizeof(piggyback_size), piggyback_size, piggybacked, &msg->srcAddr);
        }

        // Destination is other protocol
        if(msg->Proto_id != DISCOVERY_FRAMEWORK_PROTO_ID) {
            filterAndDeliver(msg);
        }

        processed = true;
    }
    // DownStream msg
    else {
        // Piggyback an hello
        DF_piggybackDiscovery(f_state, msg);

        // Insert into dispatcher queue
        DF_dispatchMessage(f_state->dispatcher_queue, msg);
        processed = true;
    }

    return processed;
}

static bool processRequest(discovery_framework_state* f_state, YggRequest* request) {

    if( request->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID ) {
        if( request->request == REQUEST ) {

            // Get Stats Request
            if( request->request_type == REQ_DISCOVERY_FRAMEWORK_STATS ) {
                    DF_uponStatsRequest(f_state, request);
                    return true;
                }
            }

            return false;
        }
        else {
            char s[100];
            sprintf(s, "Received %s from protocol %d meant for protocol %d", (request->request == REPLY ? "relpy" : "request") , request->proto_origin, request->proto_dest);

            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ERROR", s);

            return true;
        }
}

static bool processEvent(discovery_framework_state* f_state, YggEvent* event) {

    if( event->proto_dest == DISCOVERY_FRAMEWORK_PROTO_ID ) {
        return false;
    }
    else {
        char s[100];
        sprintf(s, "Received event from protocol %d meant for protocol %d", event->proto_origin, event->proto_dest);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ERROR", s);

        return false;
    }
}

discovery_framework_args* new_discovery_framework_args(DiscoveryAlgorithm* algorithm,     unsigned int hello_misses, unsigned int hack_misses, unsigned long neigh_hold_time_s, unsigned long max_jitter_ms, unsigned long period_margin_ms, unsigned int announce_transition_period_n, bool ignore_zero_seq, double lq_epsilon, double lq_threshold, double traffic_threshold, double traffic_epsilon, unsigned int discov_env_refresh_period_s, unsigned int traffic_n_bucket, unsigned int traffic_bucket_duration_s, unsigned int churn_n_bucket, unsigned int churn_bucket_duration_s, char* traffic_window_type, char* churn_window_type, double churn_epsilon, double neigh_density_epsilon) {
    discovery_framework_args* args = malloc(sizeof(discovery_framework_args));

    args->algorithm = algorithm;

    args->hello_misses = hello_misses;
    args->hack_misses = hack_misses;

    args->neigh_hold_time_s = neigh_hold_time_s;
    args->max_jitter_ms = max_jitter_ms;
    args->period_margin_ms = max_jitter_ms;
    args->announce_transition_period_n = announce_transition_period_n;

    args->ignore_zero_seq = ignore_zero_seq;

    args->lq_epsilon = lq_epsilon;
    args->lq_threshold = lq_threshold;
    args->traffic_threshold = traffic_threshold;
    args->traffic_epsilon = traffic_epsilon;

    args->discov_env_refresh_period_s = discov_env_refresh_period_s;
    args->traffic_n_bucket = traffic_n_bucket;
    args->traffic_bucket_duration_s = traffic_bucket_duration_s;
    args->churn_n_bucket = churn_n_bucket;
    args->churn_bucket_duration_s = churn_bucket_duration_s;
    strcpy(args->traffic_window_type, traffic_window_type);
    strcpy(args->churn_window_type, churn_window_type);
    args->churn_epsilon = churn_epsilon;
    args->neigh_density_epsilon = neigh_density_epsilon;

    return args;
}

discovery_framework_args* default_discovery_framework_args() {

    DiscoveryAlgorithm* algorithm = newDiscoveryAlgorithm(
        PeriodicJointDiscovery(true, true, true, true, true, true),    // Discovery Pattern
        StaticDiscoveryPeriod(5, 5),                        // Discovery Period
        EMALinkQuality(0.5, 0.7, 5, 5),                    // LinkQuality
        SimpleDiscoveryMessage()                              // Discovery Message
    );

    return new_discovery_framework_args(
        algorithm, //DiscoveryAlgorithm* algorithm,
        3, //unsigned int hello_misses,
        2, //unsigned int hack_misses,
        15, //unsigned long neigh_hold_time_s,
        500, //unsigned long max_jitter_ms,
        500, //unsigned long period_margin_ms,
        3, //unsigned int announce_transition_period_n,
        true, //bool ignore_zero_seq,
        0.05, //double lq_epsilon,
        0.3, //double lq_threshold,
        1.0, //double traffic_threshold,
        0.25, //double traffic_epsilon,
        1, //unsigned int discov_env_refresh_period_s,
        5, //unsigned int traffic_n_bucket,
        5, //unsigned int traffic_bucket_duration_s,
        5, //unsigned int churn_n_bucket,
        5, //unsigned int churn_bucket_duration_s,
        "ema 0.65", //char* traffic_window_type,
        "ema 0.65", //char* churn_window_type,
        0.1, //double churn_epsilon,
        0.1 //double neigh_density_epsilon
    );
}

static LinkQuality* parse_lq(char* value);
static DiscoveryPeriod* parse_d_period(char* value);
static DiscoveryMessage* parse_d_message(char* value);
static DiscoveryPattern* parse_d_pattern(char* value);

discovery_framework_args* load_discovery_framework_args(const char* file_path) {
    list* order = list_init();
    hash_table* configs = parse_configs_order(file_path, &order);

    if(configs == NULL) {
        char str[100];
        sprintf(str, "Config file %s not found!", file_path);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
        ygg_logflush();

        exit(-1);
    }

    discovery_framework_args* args = default_discovery_framework_args();

    for(list_item* it = order->head->data; it; it = it->next) {
        char* key = (char*)it->data;
        char* value = (char*)hash_table_find_value(configs, key);

        if( value != NULL ) {
            if( strcmp(key, "algorithm") == 0 ) {
                // TODO
            } else if( strcmp(key, "d_message") == 0 ) {
                DiscoveryMessage* new_d_mesage = parse_d_message(value);
                DA_setDiscoveryMessage(args->algorithm, new_d_mesage);
            } else if( strcmp(key, "d_period") == 0 ) {
                DiscoveryPeriod* new_d_period = parse_d_period(value);
                DA_setDiscoveryPeriod(args->algorithm, new_d_period);
            } else if( strcmp(key, "d_pattern") == 0 ) {
                DiscoveryPattern* new_d_pattern = parse_d_pattern(value);
                DA_setDiscoveryPattern(args->algorithm, new_d_pattern);
            } else if( strcmp(key, "lq_metric") == 0 ) {
                LinkQuality* new_lq_metric = parse_lq(value);
                DA_setLinkQuality(args->algorithm, new_lq_metric);
            } else if( strcmp(key, "hello_misses") == 0  ) {
                args->hello_misses = strtol(value, NULL, 10);
            } else if( strcmp(key, "hack_misses") == 0  ) {
                args->hack_misses = strtol(value, NULL, 10);
            } else if( strcmp(key, "neigh_hold_time_s") == 0  ) {
                args->neigh_hold_time_s = strtol(value, NULL, 10);
            } else if( strcmp(key, "max_jitter_ms") == 0  ) {
                args->max_jitter_ms = strtol(value, NULL, 10);
            } else if( strcmp(key, "period_margin_ms") == 0  ) {
                args->period_margin_ms = strtol(value, NULL, 10);
            } else if( strcmp(key, "announce_transition_period_n") == 0  ) {
                args->announce_transition_period_n = strtol(value, NULL, 10);
            } else if( strcmp(key, "ignore_zero_seq") == 0  ) {
                args->ignore_zero_seq = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;
            } else if( strcmp(key, "lq_epsilon") == 0  ) {
                args->lq_epsilon = strtod(value, NULL);
            } else if( strcmp(key, "lq_threshold") == 0  ) {
                args->lq_threshold = strtod(value, NULL);
            } else if( strcmp(key, "traffic_threshold") == 0  ) {
                args->traffic_threshold = strtod(value, NULL);
            } else if( strcmp(key, "traffic_epsilon") == 0  ) {
                args->traffic_epsilon = strtod(value, NULL);
            } else if( strcmp(key, "discov_env_refresh_period_s") == 0  ) {
                args->discov_env_refresh_period_s = strtol(value, NULL, 10);
            } else if( strcmp(key, "traffic_n_bucket") == 0  ) {
                args->traffic_n_bucket = strtol(value, NULL, 10);
            } else if( strcmp(key, "traffic_bucket_duration_s") == 0  ) {
                args->traffic_bucket_duration_s = strtol(value, NULL, 10);
            } else if( strcmp(key, "churn_n_bucket") == 0  ) {
                args->churn_n_bucket = strtol(value, NULL, 10);
            } else if( strcmp(key, "churn_bucket_duration_s") == 0  ) {
                args->churn_bucket_duration_s = strtol(value, NULL, 10);
            } else if( strcmp(key, "traffic_window_type") == 0  ) {
                strcpy(args->traffic_window_type, value);
            } else if( strcmp(key, "churn_window_type") == 0  ) {
                strcpy(args->churn_window_type, value);
            } else if( strcmp(key, "churn_epsilon") == 0  ) {
                args->churn_epsilon = strtod(value, NULL);
            } else if( strcmp(key, "neigh_density_epsilon") == 0  ) {
                args->neigh_density_epsilon = strtod(value, NULL);
            } else {
                char str[50];
                sprintf(str, "Unknown Config %s = %s", key, value);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
            }
        } else {
            char str[50];
            sprintf(str, "Empty Config %s", key);
            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
        }

    }

    // Clean
    hash_table_delete(configs);
    list_delete(order);

    return args;
}

static LinkQuality* parse_lq(char* value) {
    LinkQuality* lq_metric = NULL;

    char* name = NULL;
    char* ptr = NULL;
    char* token  = strtok_r(value, " ", &ptr);

    if( strcmp(token, (name = "SMA")) == 0 || strcmp(token, (name = "SMALinkQuality")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
            double initial_quality = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
                unsigned int n_buckets = strtol(token, NULL, 10);

                if(token != NULL) {
                    unsigned int bucket_duration_s = strtol(token, NULL, 10);

        			lq_metric = SMALinkQuality(initial_quality, n_buckets, bucket_duration_s);
        		} else {
        			printf("Parameter 3 of %s not passed!\n", name);
        			exit(-1);
        		}
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
    } else if( strcmp(token, (name = "WMA")) == 0 || strcmp(token, (name = "WMALinkQuality")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
            double initial_quality = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
                unsigned int n_buckets = strtol(token, NULL, 10);

                if(token != NULL) {
                    unsigned int bucket_duration_s = strtol(token, NULL, 10);

        			lq_metric = WMALinkQuality(initial_quality, n_buckets, bucket_duration_s);
        		} else {
        			printf("Parameter 3 of %s not passed!\n", name);
        			exit(-1);
        		}
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
    } else if( strcmp(token, (name = "EMA")) == 0 || strcmp(token, (name = "EMALinkQuality")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
            double initial_quality = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                double scalling = strtod(token, NULL);

                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
                    unsigned int n_buckets = strtol(token, NULL, 10);

                    if(token != NULL) {
                        unsigned int bucket_duration_s = strtol(token, NULL, 10);

            			lq_metric = EMALinkQuality(initial_quality, scalling, n_buckets, bucket_duration_s);
            		} else {
            			printf("Parameter 4 of %s not passed!\n", name);
            			exit(-1);
            		}
        		} else {
        			printf("Parameter 3 of %s not passed!\n", name);
        			exit(-1);
        		}
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
    } else {
        printf("Unrecognized LinkQuality! \n");
		exit(-1);
    }

    return lq_metric;
}

static DiscoveryPeriod* parse_d_period(char* value) {
    DiscoveryPeriod* d_period = NULL;

    char* name = NULL;
    char* ptr = NULL;
    char* token  = strtok_r(value, " ", &ptr);

    if( strcmp(token, (name = "Static")) == 0 || strcmp(token, (name = "StaticDiscoveryPeriod")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned int initial_hello_period_s = strtol(token, NULL, 10);
            assert(initial_hello_period_s < 255);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                unsigned int initial_hack_period_s = strtol(token, NULL, 10);
                assert(initial_hack_period_s < 255);

                d_period = StaticDiscoveryPeriod(initial_hello_period_s, initial_hack_period_s);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else {
        printf("Unrecognized DiscoveryPeriod! \n");
        exit(-1);
    }

    return d_period;
}

static DiscoveryMessage* parse_d_message(char* value) {
    DiscoveryMessage* d_message = NULL;

    char* name = NULL;
    char* ptr = NULL;
    char* token  = strtok_r(value, " ", &ptr);

    if( strcmp(token, (name = "Simple")) == 0 || strcmp(token, (name = "SimpleDiscoveryMessage")) == 0 ) {
        d_message = SimpleDiscoveryMessage();
    } else if( strcmp(token, (name = "OLSR")) == 0 || strcmp(token, (name = "OLSRDiscoveryMessage")) == 0 ) {
        d_message = SimpleDiscoveryMessage();
    } else {
        printf("Unrecognized DiscoveryMessage! \n");
        exit(-1);
    }

    return d_message;
}

static DiscoveryPattern* parse_d_pattern(char* value) {
    DiscoveryPattern* d_pattern = NULL;

    char* name = NULL;
    char* ptr = NULL;
    char* token  = strtok_r(value, " ", &ptr);

    if( strcmp(token, (name = "NoDiscovery")) == 0 ) {
        d_pattern = NoDiscovery();
    } else if( strcmp(token, (name = "PassiveDiscovery")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            PiggybackType hello_piggyback_type;
            if( strcmp(token, "NO_PIGGYBACK") == 0 ) {
                hello_piggyback_type = NO_PIGGYBACK;
            } else if( strcmp(token, "PIGGYBACK_ON_UNICAST_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_UNICAST_TRAFFIC;
            } else if( strcmp(token, "PIGGYBACK_ON_BROADCAST_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_BROADCAST_TRAFFIC;
            } else if( strcmp(token, "PIGGYBACK_ON_DISCOVERY_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_DISCOVERY_TRAFFIC;
            } else if( strcmp(token, "PIGGYBACK_ON_ALL_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_ALL_TRAFFIC;
            } else {
                assert(false);
            }

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                PiggybackType hack_piggyback_types;
                if( strcmp(token, "NO_PIGGYBACK") == 0 ) {
                    hack_piggyback_types = NO_PIGGYBACK;
                } else if( strcmp(token, "PIGGYBACK_ON_UNICAST_TRAFFIC") == 0 ) {
                    hack_piggyback_types = PIGGYBACK_ON_UNICAST_TRAFFIC;
                } else if( strcmp(token, "PIGGYBACK_ON_BROADCAST_TRAFFIC") == 0 ) {
                    hack_piggyback_types = PIGGYBACK_ON_BROADCAST_TRAFFIC;
                } else if( strcmp(token, "PIGGYBACK_ON_DISCOVERY_TRAFFIC") == 0 ) {
                    hack_piggyback_types = PIGGYBACK_ON_DISCOVERY_TRAFFIC;
                } else if( strcmp(token, "PIGGYBACK_ON_ALL_TRAFFIC") == 0 ) {
                    hack_piggyback_types = PIGGYBACK_ON_ALL_TRAFFIC;
                } else {
                    assert(false);
                }

                d_pattern = PassiveDiscovery(hello_piggyback_type, hack_piggyback_types);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if( strcmp(token, (name = "HybridHelloDiscovery")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            PiggybackType hello_piggyback_type;
            if( strcmp(token, "NO_PIGGYBACK") == 0 ) {
                hello_piggyback_type = NO_PIGGYBACK;
            } else if( strcmp(token, "PIGGYBACK_ON_UNICAST_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_UNICAST_TRAFFIC;
            } else if( strcmp(token, "PIGGYBACK_ON_BROADCAST_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_BROADCAST_TRAFFIC;
            } else if( strcmp(token, "PIGGYBACK_ON_DISCOVERY_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_DISCOVERY_TRAFFIC;
            } else if( strcmp(token, "PIGGYBACK_ON_ALL_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_ALL_TRAFFIC;
            } else {
                assert(false);
            }

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_new_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_lost_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_update_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_new_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_lost_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                                token = strtok_r(NULL, " ", &ptr);
                                if(token != NULL) {
                                    bool react_to_update_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                                    d_pattern = HybridHelloDiscovery(hello_piggyback_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
                                } else {
                                    printf("Parameter 7 of %s not passed!\n", name);
                                    exit(-1);
                                }
                            } else {
                                printf("Parameter 6 of %s not passed!\n", name);
                                exit(-1);
                            }
                        } else {
                            printf("Parameter 5 of %s not passed!\n", name);
                            exit(-1);
                        }
                    } else {
                        printf("Parameter 4 of %s not passed!\n", name);
                        exit(-1);
                    }
                } else {
                    printf("Parameter 3 of %s not passed!\n", name);
                    exit(-1);
                }
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if( strcmp(token, (name = "PeriodicHelloDiscovery")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {

            bool react_to_new_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_lost_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_update_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_new_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_lost_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_update_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                                d_pattern = PeriodicHelloDiscovery(react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
                            } else {
                                printf("Parameter 6 of %s not passed!\n", name);
                                exit(-1);
                            }
                        } else {
                            printf("Parameter 5 of %s not passed!\n", name);
                            exit(-1);
                        }
                    } else {
                        printf("Parameter 4 of %s not passed!\n", name);
                        exit(-1);
                    }
                } else {
                    printf("Parameter 3 of %s not passed!\n", name);
                    exit(-1);
                }
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if( strcmp(token, (name = "PeriodicJointDiscovery")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {

            bool react_to_new_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_lost_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_update_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_new_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_lost_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_update_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                                d_pattern = PeriodicJointDiscovery(react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
                            } else {
                                printf("Parameter 6 of %s not passed!\n", name);
                                exit(-1);
                            }
                        } else {
                            printf("Parameter 5 of %s not passed!\n", name);
                            exit(-1);
                        }
                    } else {
                        printf("Parameter 4 of %s not passed!\n", name);
                        exit(-1);
                    }
                } else {
                    printf("Parameter 3 of %s not passed!\n", name);
                    exit(-1);
                }
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if( strcmp(token, (name = "PeriodicDisjointDiscovery")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {

            bool react_to_new_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_lost_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_update_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_new_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_lost_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_update_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                                d_pattern = PeriodicDisjointDiscovery(react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
                            } else {
                                printf("Parameter 6 of %s not passed!\n", name);
                                exit(-1);
                            }
                        } else {
                            printf("Parameter 5 of %s not passed!\n", name);
                            exit(-1);
                        }
                    } else {
                        printf("Parameter 4 of %s not passed!\n", name);
                        exit(-1);
                    }
                } else {
                    printf("Parameter 3 of %s not passed!\n", name);
                    exit(-1);
                }
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if( strcmp(token, (name = "HybridDisjointDiscovery")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            PiggybackType hello_piggyback_type;
            if( strcmp(token, "NO_PIGGYBACK") == 0 ) {
                hello_piggyback_type = NO_PIGGYBACK;
            } else if( strcmp(token, "PIGGYBACK_ON_UNICAST_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_UNICAST_TRAFFIC;
            } else if( strcmp(token, "PIGGYBACK_ON_BROADCAST_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_BROADCAST_TRAFFIC;
            } else if( strcmp(token, "PIGGYBACK_ON_DISCOVERY_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_DISCOVERY_TRAFFIC;
            } else if( strcmp(token, "PIGGYBACK_ON_ALL_TRAFFIC") == 0 ) {
                hello_piggyback_type = PIGGYBACK_ON_ALL_TRAFFIC;
            } else {
                assert(false);
            }

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                PiggybackType hack_piggyback_type;
                if( strcmp(token, "NO_PIGGYBACK") == 0 ) {
                    hack_piggyback_type = NO_PIGGYBACK;
                } else if( strcmp(token, "PIGGYBACK_ON_UNICAST_TRAFFIC") == 0 ) {
                    hack_piggyback_type = PIGGYBACK_ON_UNICAST_TRAFFIC;
                } else if( strcmp(token, "PIGGYBACK_ON_BROADCAST_TRAFFIC") == 0 ) {
                    hack_piggyback_type = PIGGYBACK_ON_BROADCAST_TRAFFIC;
                } else if( strcmp(token, "PIGGYBACK_ON_DISCOVERY_TRAFFIC") == 0 ) {
                    hack_piggyback_type = PIGGYBACK_ON_DISCOVERY_TRAFFIC;
                } else if( strcmp(token, "PIGGYBACK_ON_ALL_TRAFFIC") == 0 ) {
                    hack_piggyback_type = PIGGYBACK_ON_ALL_TRAFFIC;
                } else {
                    assert(false);
                }

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {

                    bool react_to_new_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_lost_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_update_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_new_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                                token = strtok_r(NULL, " ", &ptr);
                                if(token != NULL) {
                                    bool react_to_lost_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                                    token = strtok_r(NULL, " ", &ptr);
                                    if(token != NULL) {
                                        bool react_to_update_2hop_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                                        d_pattern = HybridDisjointDiscovery(hello_piggyback_type, hack_piggyback_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
                                    } else {
                                        printf("Parameter 8 of %s not passed!\n", name);
                                        exit(-1);
                                    }
                                } else {
                                    printf("Parameter 7 of %s not passed!\n", name);
                                    exit(-1);
                                }
                            } else {
                                printf("Parameter 6 of %s not passed!\n", name);
                                exit(-1);
                            }
                        } else {
                            printf("Parameter 5 of %s not passed!\n", name);
                            exit(-1);
                        }
                    } else {
                        printf("Parameter 4 of %s not passed!\n", name);
                        exit(-1);
                    }
                } else {
                    printf("Parameter 3 of %s not passed!\n", name);
                    exit(-1);
                }
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if( strcmp(token, (name = "EchoDiscovery")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            HackReplyType reply_type;
            if( strcmp(token, "NO_HACK_REPLY") == 0 ) {
                reply_type = NO_HACK_REPLY;
            } else if( strcmp(token, "BROADCAST_HACK_REPLY") == 0 ) {
                reply_type = BROADCAST_HACK_REPLY;
            } else if( strcmp(token, "UNICAST_HACK_REPLY") == 0 ) {
                reply_type = UNICAST_HACK_REPLY;
            } else {
                assert(false);
            }

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool piggyback_hello_on_reply = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_new_neighbor = strcmp(value, "true") == 0 || strcmp(value, "True") == 0;

                    d_pattern = EchoDiscovery(reply_type, piggyback_hello_on_reply, react_to_new_neighbor);
                } else {
                    printf("Parameter 3 of %s not passed!\n", name);
                    exit(-1);
                }
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else {
        printf("Unrecognized DiscoveryMessage! \n");
        exit(-1);
    }

    return d_pattern;
}
