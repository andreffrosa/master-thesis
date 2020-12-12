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

#include "utility/my_sys.h"

#include <assert.h>

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

    for(list_item* it = order->head; it; it = it->next) {
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
    } else if( strcmp(token, (name = "LENWB")) == 0 || strcmp(token, (name = "LENWBDiscoveryMessage")) == 0 ) {
        d_message = LENWBDiscoveryMessage();
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
                bool react_to_new_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                d_pattern = HybridHelloDiscovery(hello_piggyback_type, react_to_new_neighbor);
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
            bool react_to_new_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

            d_pattern = PeriodicHelloDiscovery(react_to_new_neighbor);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if( strcmp(token, (name = "PeriodicJointDiscovery")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {

            bool react_to_new_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_lost_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_update_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_new_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_lost_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_update_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

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

            bool react_to_new_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_lost_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_update_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_new_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_lost_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_update_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

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

                    bool react_to_new_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_lost_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_update_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_new_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                                token = strtok_r(NULL, " ", &ptr);
                                if(token != NULL) {
                                    bool react_to_lost_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                                    token = strtok_r(NULL, " ", &ptr);
                                    if(token != NULL) {
                                        bool react_to_update_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

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
    } else if( strcmp(token, (name = "HybridHelloPeriodicHackDiscovery")) == 0 ) {
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
                bool react_to_new_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_lost_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_update_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_new_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_lost_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                                token = strtok_r(NULL, " ", &ptr);
                                if(token != NULL) {
                                    bool react_to_update_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                                    d_pattern = HybridHelloPeriodicHackDiscovery(hello_piggyback_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
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
    } else if( strcmp(token, (name = "PeriodicHelloHybridHackDiscovery")) == 0 ) {
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
                bool react_to_new_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_lost_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_update_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_new_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_lost_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                                token = strtok_r(NULL, " ", &ptr);
                                if(token != NULL) {
                                    bool react_to_update_2hop_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                                    d_pattern = PeriodicHelloHybridHackDiscovery(hack_piggyback_type, react_to_new_neighbor, react_to_lost_neighbor, react_to_update_neighbor, react_to_new_2hop_neighbor, react_to_lost_2hop_neighbor, react_to_update_2hop_neighbor);
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
                bool piggyback_hello_on_reply = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_new_neighbor = strcmp(token, "true") == 0 || strcmp(token, "True") == 0;

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
