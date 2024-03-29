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
#include "utility/my_string.h"

#include <assert.h>

discovery_framework_args* new_discovery_framework_args(DiscoveryAlgorithm* algorithm, unsigned int hello_misses, unsigned int hack_misses, unsigned long neigh_hold_time_s, unsigned long max_jitter_ms, unsigned long period_margin_ms, unsigned int announce_transition_period_n, unsigned long min_hello_interval_ms, unsigned long min_hack_interval_ms, bool ignore_zero_seq, double lq_epsilon, int lq_precision, double lq_threshold, double traffic_epsilon, int traffic_precision, double traffic_threshold, unsigned int discov_env_refresh_period_s, unsigned int traffic_n_bucket, unsigned int traffic_bucket_duration_s, unsigned int churn_n_bucket, unsigned int churn_bucket_duration_s, char* traffic_window_type, char* churn_window_type, double churn_epsilon, int churn_precision, double neigh_density_epsilon, int neigh_density_precision, bool toggle_env) {

    discovery_framework_args* args = malloc(sizeof(discovery_framework_args));

    args->algorithm = algorithm;

    args->hello_misses = hello_misses;
    args->hack_misses = hack_misses;

    args->neigh_hold_time_s = neigh_hold_time_s;
    args->max_jitter_ms = max_jitter_ms;
    args->period_margin_ms = max_jitter_ms;
    args->announce_transition_period_n = announce_transition_period_n;

    args->min_hello_interval_ms = min_hello_interval_ms;
    args->min_hack_interval_ms = min_hack_interval_ms;

    args->ignore_zero_seq = ignore_zero_seq;

    args->lq_epsilon = lq_epsilon;
    args->lq_threshold = lq_threshold;
    args->lq_precision = lq_precision;

    args->traffic_threshold = traffic_threshold;
    args->traffic_epsilon = traffic_epsilon;
    args->traffic_precision = traffic_precision;

    args->discov_env_refresh_period_s = discov_env_refresh_period_s;
    args->traffic_n_bucket = traffic_n_bucket;
    args->traffic_bucket_duration_s = traffic_bucket_duration_s;
    args->churn_n_bucket = churn_n_bucket;
    args->churn_bucket_duration_s = churn_bucket_duration_s;
    strcpy(args->traffic_window_type, traffic_window_type);
    strcpy(args->churn_window_type, churn_window_type);

    args->churn_epsilon = churn_epsilon;
    args->churn_precision = churn_precision;

    args->neigh_density_epsilon = neigh_density_epsilon;
    args->neigh_density_precision = neigh_density_precision;

    args->toggle_env = toggle_env;

    return args;
}

discovery_framework_args* default_discovery_framework_args() {

    DiscoveryAlgorithm* algorithm = newDiscoveryAlgorithm(
        PeriodicJointDiscovery(RESET_PERIODIC, true, true, true, true),     // Discovery Pattern
        StaticDiscoveryPeriod(5, 5),                                        // Discovery Period
        EMALinkQuality(0.5, 0.7, 5, 5),                                     // LinkQuality
        NoAdmission(),                                                      // LinkAdmission
        EmptyDiscoveryContext()                                             // Discovery Message
    );

    return new_discovery_framework_args(
        algorithm, //DiscoveryAlgorithm* algorithm,
        3, //unsigned int hello_misses,
        2, //unsigned int hack_misses,
        15, //unsigned long neigh_hold_time_s,
        500, //unsigned long max_jitter_ms,
        500, //unsigned long period_margin_ms,
        3, //unsigned int announce_transition_period_n,
        1000, //unsigned long min_hello_interval_ms,
        1000, //unsigned long min_hack_interval_ms
        true, //bool ignore_zero_seq,
        0.05, //double lq_epsilon,
        -2, //int lq_precision,
        0.2, //double lq_threshold,
        0.05, //double traffic_epsilon,
        -2, //double traffic_precision,
        1.0, //double traffic_threshold,
        1, //unsigned int discov_env_refresh_period_s,
        5, //unsigned int traffic_n_bucket,
        5, //unsigned int traffic_bucket_duration_s,
        5, //unsigned int churn_n_bucket,
        5, //unsigned int churn_bucket_duration_s,
        "ema 0.5", //char* traffic_window_type,
        "ema 0.5", //char* churn_window_type,
        0.1, //double churn_epsilon,
        -2, //double churn_precision,
        0.1, //double neigh_density_epsilon,
        -2, //double neigh_density_precision
        false // toggle_env
    );
}

static LinkQuality* parse_lq(char* value, bool nested);
static LinkAdmission* parse_la(char* value, bool nested);
static DiscoveryPeriod* parse_d_period(char* value, bool nested);
static DiscoveryContext* parse_d_context(char* value, bool nested);
static DiscoveryPattern* parse_d_pattern(char* value, bool nested);
static PiggybackFilter* parse_piggyback_filter(char* value, bool nested);
static PeriodicType parse_periodic_type(char* token);

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
            } else if( strcmp(key, "d_context") == 0 ) {
                DiscoveryContext* new_d_context = parse_d_context(value, false);
                DA_setDiscoveryContext(args->algorithm, new_d_context);
            } else if( strcmp(key, "d_period") == 0 ) {
                DiscoveryPeriod* new_d_period = parse_d_period(value, false);
                DA_setDiscoveryPeriod(args->algorithm, new_d_period);
            } else if( strcmp(key, "d_pattern") == 0 ) {
                DiscoveryPattern* new_d_pattern = parse_d_pattern(value, false);
                DA_setDiscoveryPattern(args->algorithm, new_d_pattern);
            } else if( strcmp(key, "lq_metric") == 0 ) {
                LinkQuality* new_lq_metric = parse_lq(value, false);
                DA_setLinkQuality(args->algorithm, new_lq_metric);
            } else if( strcmp(key, "la_policy") == 0 ) {
                LinkAdmission* new_la_policy = parse_la(value, false);
                DA_setLinkAdmission(args->algorithm, new_la_policy);
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
            } else if( strcmp(key, "min_hello_interval_ms") == 0  ) {
                args->min_hello_interval_ms = strtol(value, NULL, 10);
            } else if( strcmp(key, "min_hack_interval_ms") == 0  ) {
                args->min_hack_interval_ms = strtol(value, NULL, 10);
            } else if( strcmp(key, "ignore_zero_seq") == 0  ) {
                args->ignore_zero_seq = parse_bool(value);
            } else if( strcmp(key, "lq_epsilon") == 0  ) {
                args->lq_epsilon = strtod(value, NULL);
            } else if( strcmp(key, "lq_precision") == 0  ) {
                args->lq_precision = strtol(value, NULL, 10);
            } else if( strcmp(key, "lq_threshold") == 0  ) {
                args->lq_threshold = strtod(value, NULL);
            } else if( strcmp(key, "traffic_threshold") == 0  ) {
                args->traffic_threshold = strtod(value, NULL);
            } else if( strcmp(key, "traffic_epsilon") == 0  ) {
                args->traffic_epsilon = strtod(value, NULL);
            } else if( strcmp(key, "traffic_precision") == 0  ) {
                args->traffic_precision = strtol(value, NULL, 10);
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
            } else if( strcmp(key, "churn_precision") == 0  ) {
                args->churn_precision = strtol(value, NULL, 10);
            } else if( strcmp(key, "neigh_density_epsilon") == 0  ) {
                args->neigh_density_epsilon = strtod(value, NULL);
            } else if( strcmp(key, "neigh_density_precision") == 0  ) {
                args->neigh_density_precision = strtol(value, NULL, 10);
            } else if( strcmp(key, "toggle_env") == 0  ) {
                args->toggle_env = parse_bool(value);
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

static LinkQuality* parse_lq(char* value, bool nested) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

    if( strcmp(token, (name = "SMA")) == 0 || strcmp(token, (name = "SMALinkQuality")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double initial_quality = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                unsigned int n_buckets = strtol(token, NULL, 10);

                if(token != NULL) {
                    unsigned int bucket_duration_s = strtol(token, NULL, 10);

                    return SMALinkQuality(initial_quality, n_buckets, bucket_duration_s);
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

                    return WMALinkQuality(initial_quality, n_buckets, bucket_duration_s);
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

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        unsigned int bucket_duration_s = strtol(token, NULL, 10);

                        return EMALinkQuality(initial_quality, scalling, n_buckets, bucket_duration_s);
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
    } else if( strcmp(token, (name = "SW")) == 0 || strcmp(token, (name = "SWLinkQuality")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double initial_quality = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                unsigned int window_size = strtol(token, NULL, 10);

                return SlidingWindowLinkQuality(initial_quality, window_size);
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
}

static LinkAdmission* parse_la(char* value, bool nested) {

        if(value == NULL) {
            printf("No parameter passed");
            exit(-1);
        }

        char* name = NULL;
        char* ptr = NULL;
        char* token = NULL;

        int len = strlen(value);
        char str[len+1];

        if(!nested) {
            memcpy(str, value, len+1);
            token  = strtok_r(str, " ", &ptr);
        } else {
            token  = value;
        }

        if( strcmp(token, (name = "No")) == 0 || strcmp(token, (name = "NoAdmission")) == 0 ) {
            return NoAdmission();
        } else if( strcmp(token, (name = "Age")) == 0 || strcmp(token, (name = "AgeAdmission")) == 0 ) {
            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                unsigned int min_hellos = strtol(token, NULL, 10);

                return AgeAdmission(min_hellos);
            } else {
                printf("Parameter 1 of %s not passed!\n", name);
                exit(-1);
            }
        } else if( strcmp(token, (name = "Hysteresis")) == 0 || strcmp(token, (name = "HysteresisAdmission")) == 0 ) {
            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                double hyst_reject = strtod(token, NULL);

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    double hyst_accept = strtod(token, NULL);

                    return HysteresisAdmission(hyst_reject, hyst_accept);
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
}

static DiscoveryPeriod* parse_d_period(char* value, bool nested) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    DiscoveryPeriod* d_period = NULL;

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

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

static DiscoveryContext* parse_d_context(char* value, bool nested) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

    if( strcmp(token, (name = "Empty")) == 0 || strcmp(token, (name = "EmptyDiscoveryContext")) == 0 ) {
        return EmptyDiscoveryContext();
    } else if( strcmp(token, (name = "OLSR")) == 0 || strcmp(token, (name = "OLSRDiscoveryContext")) == 0 ) {
        return OLSRDiscoveryContext();
    } else if( strcmp(token, (name = "LENWB")) == 0 || strcmp(token, (name = "LENWBDiscoveryContext")) == 0 ) {
        return LENWBDiscoveryContext();
    } else {
        printf("Unrecognized DiscoveryContext! \n");
        exit(-1);
    }

}

static DiscoveryPattern* parse_d_pattern(char* value, bool nested) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

    if( strcmp(token, (name = "NoDiscovery")) == 0 ) {
        return NoDiscovery();
    } else if( strcmp(token, (name = "PassiveDiscovery")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            PiggybackFilter* hello_piggyback_filter = parse_piggyback_filter(token, true);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                PiggybackFilter* hack_piggyback_filter = parse_piggyback_filter(token, true);

                return PassiveDiscovery(hello_piggyback_filter, hack_piggyback_filter);
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
            PiggybackFilter* hello_piggyback_filter = parse_piggyback_filter(token, true);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_new_neighbor = parse_bool(token);

                return HybridHelloDiscovery(hello_piggyback_filter, react_to_new_neighbor);
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
            PeriodicType periodic_type = parse_periodic_type(token);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_new_neighbor = parse_bool(token);

                return PeriodicHelloDiscovery(periodic_type, react_to_new_neighbor);
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
            PeriodicType periodic_type = parse_periodic_type(token);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_new_neighbor = parse_bool(token);

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_update_neighbor = parse_bool(token);

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_lost_neighbor = parse_bool(token);

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_context_updates = parse_bool(token);

                            return PeriodicJointDiscovery(periodic_type, react_to_new_neighbor, react_to_update_neighbor, react_to_lost_neighbor, react_to_context_updates);
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
            PeriodicType periodic_type = parse_periodic_type(token);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                bool react_to_new_neighbor = parse_bool(token);

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_update_neighbor = parse_bool(token);

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_lost_neighbor = parse_bool(token);

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_context_updates = parse_bool(token);

                            return PeriodicDisjointDiscovery(periodic_type, react_to_new_neighbor, react_to_update_neighbor, react_to_lost_neighbor, react_to_context_updates);
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
            PiggybackFilter* hello_piggyback_filter = parse_piggyback_filter(token, true);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                PiggybackFilter* hack_piggyback_filter = parse_piggyback_filter(token, true);

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_new_neighbor = parse_bool(token);

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_update_neighbor = parse_bool(token);

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_lost_neighbor = parse_bool(token);

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_context_updates = parse_bool(token);

                                return HybridDisjointDiscovery(hello_piggyback_filter, hack_piggyback_filter, react_to_new_neighbor, react_to_update_neighbor, react_to_lost_neighbor, react_to_context_updates);

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
            PiggybackFilter* hello_piggyback_filter = parse_piggyback_filter(token, true);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                PeriodicType hack_periodic_type = parse_periodic_type(token);

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_new_neighbor = parse_bool(token);

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_update_neighbor = parse_bool(token);

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_lost_neighbor = parse_bool(token);

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_context_updates = parse_bool(token);

                                return HybridHelloPeriodicHackDiscovery(hello_piggyback_filter, hack_periodic_type, react_to_new_neighbor, react_to_update_neighbor, react_to_lost_neighbor, react_to_context_updates);
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
            PiggybackFilter* hack_piggyback_filter = parse_piggyback_filter(token, true);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                PeriodicType hello_periodic_type = parse_periodic_type(token);

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    bool react_to_new_neighbor = parse_bool(token);

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        bool react_to_update_neighbor = parse_bool(token);

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            bool react_to_lost_neighbor = parse_bool(token);

                            token = strtok_r(NULL, " ", &ptr);
                            if(token != NULL) {
                                bool react_to_context_updates = parse_bool(token);

                                return PeriodicHelloHybridHackDiscovery(hack_piggyback_filter, hello_periodic_type, react_to_new_neighbor, react_to_update_neighbor, react_to_lost_neighbor, react_to_context_updates);
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
                bool piggyback_hello_on_reply_if_new_neighbor = parse_bool(token);

                return EchoDiscovery(reply_type, piggyback_hello_on_reply_if_new_neighbor);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else {
        printf("Unrecognized DiscoveryContext! \n");
        exit(-1);
    }

}

static PiggybackFilter* parse_piggyback_filter(char* value, bool nested) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

    if( strcmp(token, (name = "NoPiggyback")) == 0 ) {
        return NoPiggyback();
    } else if( strcmp(token, (name = "PiggybackOnUnicast")) == 0 ) {
        return PiggybackOnUnicast();
    } else if( strcmp(token, (name = "PiggybackOnBroadcast")) == 0 ) {
        return PiggybackOnBroadcast();
    } else if( strcmp(token, (name = "PiggybackOnDiscovery")) == 0 ) {
        return PiggybackOnDiscovery();
    } else if( strcmp(token, (name = "PiggybackOnAll")) == 0 ) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            bool convert_to_broadcast = parse_bool(token);

            return PiggybackOnAll(convert_to_broadcast);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if( strcmp(token, (name = "PiggybackOnNewNeighbor")) == 0 ) {
        return PiggybackOnNewNeighbor();
    } else {
        printf("Unrecognized PiggybackFilter! \n");
        exit(-1);
    }

}

static PeriodicType parse_periodic_type(char* token) {
    if(token == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;

    if( strcmp(token, (name = "NO_PERIODIC")) == 0 ) {
        return NO_PERIODIC;
    } else if( strcmp(token, (name = "STATIC_PERIODIC")) == 0 ) {
        return STATIC_PERIODIC;
    } else if( strcmp(token, (name = "RESET_PERIODIC")) == 0 ) {
        return RESET_PERIODIC;
    } else {
        printf("Unrecognized PeriodicType! \n");
        exit(-1);
    }

}
