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

#include "process_args.h"

#include <limits.h>

#include "utility/my_sys.h"
#include "utility/my_time.h"
#include "utility/my_misc.h"
#include "utility/my_string.h"

struct option long_options[] = {
         { "file", required_argument, NULL, 'f' },
         { "rcv_only", required_argument, NULL, 'R' },
         { "max_bcasts", required_argument, NULL, 'b' },
         { "broadcast_type", required_argument, NULL, 'T' },
         { "initial_grace_period_s", required_argument, NULL, 'i' },
         { "final_grace_period_s", required_argument, NULL, 'F' },
         //{ "init_timer", required_argument, NULL, 'i' },
         //{ "periodic_timer", required_argument, NULL, 'l' },
         { "duration", required_argument, NULL, 'd' },
         //{ "bcast_prob", required_argument, NULL, 'p' },
         { "overlay", required_argument, NULL, 'o' },
         { "interface", required_argument, NULL, 'I' },
         { "hostname", required_argument, NULL, 'H' },
         { "window_size", required_argument, NULL, 'w' },
         { "horizon", required_argument, NULL, 'h' },
         { "max_jitter", required_argument, NULL, 'j' },
         { "heartbeat_period", required_argument, NULL, 'x' },
         { "t_gc", required_argument, NULL, 'g' },
         { "misses", required_argument, NULL, 'm' },
         { "lq_alg", required_argument, NULL, 'q' },
         { "lq_threshold", required_argument, NULL, 'Q' },
         { "r_alg", required_argument, NULL, 'a' },
         { "r_delay", required_argument, NULL, 't' },
         { "r_policy",required_argument, NULL, 'r' },
         { "r_context", required_argument, NULL, 'c' },
         { "r_phases", required_argument, NULL, 'n' },
         { "seen_expiration_ms", required_argument, NULL, 's' },
         { "gc_interval_s", required_argument, NULL, 'G' },
         { NULL, 0, NULL, 0 }
 };

void defaultConfigValues(Configs* config) {
    config->broadcast.f_args = new_broadcast_framework_args(Flooding(500), 1*60*1000, 3*60); // 1 min 5min

    strcpy(config->broadcast.r_alg, "Flooding 100");
    strcpy(config->broadcast.r_delay, "");
    strcpy(config->broadcast.r_policy, "");
    strcpy(config->broadcast.r_context, "");

    strcpy(config->app.broadcast_type, "Exponential Constant 2.0");
    clock_gettime(CLOCK_MONOTONIC, &config->app.start_time);
    config->app.initial_grace_period_s = 15*1000; // 15 seconds
    config->app.final_grace_period_s = 15*1000; // 15 seconds

    //config->app.initial_timer_ms = 1000; // 1 s
	//config->app.periodic_timer_ms = 1000; // 1 s
	config->app.max_broadcasts = ULONG_MAX; // infinite
	config->app.exp_duration_s = 5*60; // 5 min
	config->app.rcv_only = false;
	//config->app.bcast_prob = 1.0; // 100%
	config->app.use_overlay = false;
    bzero(config->app.overlay_path, PATH_MAX);

    config->discovery.window_size = 10;
    config->discovery.d_args = malloc(sizeof(topology_discovery_args));
    new_topology_discovery_args(config->discovery.d_args, 2, 500, 5000, 60*1000, 3, OLRSQuality(0.5), 0.1);
    strcpy(config->discovery.lq_alg_str, "OLRSQuality 0.5");

    strcpy(config->app.interface_name, "wlan0");
    strcpy(config->app.hostname, "unknown");
}


void printConfigs(Configs* config) { // transformar o f_args nas strings?

    char broadcast_str[2000];
    sprintf(broadcast_str,
        "BroadcastConfigs {\n"
        "\t\tr_alg = %s\n"
        "\t\tr_delay = %s\n"
        "\t\tr_policy = %s\n"
        "\t\tr_context = %s\n"
        "\t\tr_phases = %d\n"
        "\t\tseen_expiration_ms = %lu\n"
        "\t\tgc_interval_s = %lu\n"
        "\t}\n"
        , config->broadcast.r_alg
        , config->broadcast.r_delay
        , config->broadcast.r_policy
        , config->broadcast.r_context
        , getRetransmissionPhases(config->broadcast.f_args->algorithm)
        , config->broadcast.f_args->seen_expiration_ms
        , config->broadcast.f_args->gc_interval_s
    );

    char discovery_str[2000];
    sprintf(discovery_str,
        "DiscoveryConfigs {\n"
        "\t\twindow_size = %lu s\n"
        "\t\thorizon = %d\n"
        "\t\tmax_jitter = %lu\n"
        "\t\theartbeat_period = %lu\n"
        "\t\tt_gc = %lu\n"
        "\t\tmisses = %u\n"
        "\t\tlq_alg = %s\n"
        "\t\tlq_threshold = %f\n"
        "\t}\n"
        , config->discovery.window_size
        , config->discovery.d_args->horizon
        , config->discovery.d_args->max_jitter
        , config->discovery.d_args->heartbeat_period
        , config->discovery.d_args->t_gc
        , config->discovery.d_args->misses
        , config->discovery.lq_alg_str
        , config->discovery.d_args->lq_threshold
    );

    //char start_time_str[100];
    //timespec_to_string(&config->app.start_time, start_time_str, 100, 3);

    char max_broadcasts[200];
    if(config->app.max_broadcasts == ULONG_MAX)
        sprintf(max_broadcasts, "Infinite");
    else
        sprintf(max_broadcasts, "%lu", config->app.max_broadcasts);

    char app_str[2000];
    sprintf(app_str,
        "AppConfigs {\n"
        "\t\tbroadcast_type = %s\n"
        //"\t\tstart_time = %s\n"
        "\t\tinitial_grace_period = %lu s\n"
        "\t\tfinal_grace_period = %lu s\n"
        "\t\tmax_broadcasts = %s\n"
        "\t\texp_duration_s = %lu s\n"
        "\t\trcv_only = %s\n"
        "\t\toverlay = %s\n"
        "\t\tinterface_name = %s\n"
        "\t\thostname = %s\n"
        "\t}\n"
        , config->app.broadcast_type
        //, start_time_str
        , config->app.initial_grace_period_s
        , config->app.final_grace_period_s
        , max_broadcasts
        , config->app.exp_duration_s
        , (config->app.rcv_only ? "true" : "false")
        , config->app.use_overlay ? config->app.overlay_path : "NULL"
        , config->app.interface_name
        , config->app.hostname == NULL ? "NULL" : config->app.hostname
    );

    printf("Configs {\n"
        "\t%s"
        "\t%s"
        "\t%s"
        "}\n"
        , broadcast_str
        , discovery_str
        , app_str
    );
}

void processFile(const char* file_path, Configs* config) {

    list* order = list_init();
    hash_table* table = parse_configs_order(file_path, &order);

    if(table == NULL) {
        fprintf(stderr, "Config file not found!\n");
        exit(-1);
    }

    for(list_item* it = order->head; it; it = it->next) {
        char* k = it->data;

        if(strcmp("file", k) == 0)
            continue;

        char* v =  (char*)hash_table_find_value(table, k);

        if(v != NULL){
            char val = 0;
            int i = 0;
            bool found = false;
            for(; long_options[i].name != NULL; i++) {
                if(strcmp(k, long_options[i].name) == 0) {
                    val = long_options[i].val;
                    found = true;
                    break;
                }
            }

            if(found) {
                processLine(val, i, v, config);
            } else {
                printf("Unknown config %s = %s\n", k, v);
            }
        } else {
            printf("Config %s has no value\n", k);
        }
    }

    /*for(int i = 0; long_options[i].name != NULL; i++) {
        if(strcmp("file", long_options[i].name) == 0)
            continue;

        char* v =  (char*)hash_table_find_value(table, (char*)long_options[i].name);

        if(v != NULL){
            processLine(long_options[i].val, i, v, config);
        }
    }*/

    hash_table_delete(table);
    list_delete(order);
}

void processArgs(int argc, char** argv, Configs* config) {

	int option_index = -1;
	int ch;
	opterr = 0;
	while ((ch = getopt_long(argc, argv, "f:R:b:T:i:F:d:o:I:H:w:h:j:x:g:m:q:Q:a:t:r:c:n:s:G:", long_options, &option_index)) != -1) {
        if(option_index == -1) {
            bool found = false;
            for(int i = 0; long_options[i].name != NULL; i++) {
                if(ch == long_options[i].val) {
                    option_index = i;
                    found = true;
                    break;
                }
            }
            if(!found) {
                printf("Unrecognized ch %c\n", ch);
            }
        }
        processLine(ch, option_index, optarg, config);

        option_index = -1;

		//printf("-s = %d, -t = %d\n", *silent_mode, *m_sleep);

		/*for (int index = optind; index < argc; index++)
	 printf("Non-option argument %s\n", argv[index]);*/
	}
}

void processLine(char option, int option_index, char* args, Configs* config) {

    //printf("%s: %s\n", long_options[option_index].name, args);

    //char* token;

    printf("%s = %s\n", long_options[option_index].name, args);

    switch (option) {
        case 'f':
            /*token = strtok(args, " ");

            if(token != NULL) {
                processFile(token, config);
            } else
                fprintf(stderr, "No parameter passed");
            */
            processFile(args, config);
            break;
        case 'I':
            strcpy(config->app.interface_name, args);
            break;
        case 'H':
            strcpy(config->app.hostname, args);
            break;
        case 'a':
            mountBroadcastAlgorithm(args, config);
            break;
        case 'r':
            mountRetransmissionPolicy(args, config);
            break;
        case 't':
            mountRetransmissionDelay(args, config);
            break;
        case 'c':
            mountRetransmissionContext(args, config);
            break;
        case 'n':
            setRetransmissionPhases(config->broadcast.f_args->algorithm, strtol(args, NULL, 10));
            break;
        case 's':
            config->broadcast.f_args->seen_expiration_ms = strtol(args, NULL, 10);
            break;
        case 'G':
            config->broadcast.f_args->gc_interval_s = strtol(args, NULL, 10);
            break;
        case 'b':
            config->app.max_broadcasts = (strcmp(args, "infinite") == 0) ? ULONG_MAX : (unsigned int) strtol(args, NULL, 10);
            break;
        case 'i':
            //config->app.initial_timer_ms = (unsigned long) strtol(args, NULL, 10);
            config->app.initial_grace_period_s = (unsigned long) strtol(args, NULL, 10);
            break;
        case 'F':
            //config->app.initial_timer_ms = (unsigned long) strtol(args, NULL, 10);
            config->app.final_grace_period_s = (unsigned long) strtol(args, NULL, 10);
            break;
        case 'T':
            strcpy(config->app.broadcast_type, args);
            break;
        /*case 'l':
            config->app.periodic_timer_ms = (unsigned long) strtol(args, NULL, 10);
            break;*/
        case 'd':
            config->app.exp_duration_s = (unsigned long) strtol(args, NULL, 10);
            break;
        case 'R':
            config->app.rcv_only = strcmp(args, "true") == 0 || strcmp(args, "True") == 0;
            break;
        /*case 'p':
            config->app.bcast_prob = strtod (args, NULL);
            //printf(" %f\n", prob);
            break;*/
        case 'o':
            if(strcmp(args, "NULL") == 0) {
                config->app.use_overlay = false;
            } else {
                config->app.use_overlay = true;
                memcpy(config->app.overlay_path, args, strlen(args)+1);
            }
            break;
        case 'w':
            config->discovery.window_size = (unsigned long) strtol(args, NULL, 10);
            break;
        case 'h':
            config->discovery.d_args->horizon = (unsigned long) strtol(args, NULL, 10);
            break;
        case 'j':
            config->discovery.d_args->max_jitter = (unsigned long) strtol(args, NULL, 10);
            break;
        case 'x':
            config->discovery.d_args->heartbeat_period = (unsigned long) strtol(args, NULL, 10);
            break;
        case 'g':
            config->discovery.d_args->t_gc = (unsigned long) strtol(args, NULL, 10);
            break;
        case 'm':
            config->discovery.d_args->misses = (unsigned long) strtol(args, NULL, 10);
            break;
        case 'q':
            mountLQ(args, config);
            break;
        case 'Q':
            config->discovery.d_args->lq_threshold = (unsigned long) strtol(args, NULL, 10);
            break;
        case '?':
            if ( optopt == 'a' || optopt == 'r' || optopt == 't' || optopt == 'c' || optopt == 'n' || optopt == 'b' || optopt == 'i' || optopt == 'F' || /*optopt == 'l' ||*/ optopt == 'd' || /*optopt == 'p' ||*/ optopt == 'f' || optopt == 'I' || optopt == 'R' || optopt == 'H' || optopt == 'L' || optopt == 'w' || optopt == 'h' || optopt == 'j' || optopt == 'x' || optopt == 'g' || optopt == 'm' || optopt == 'q' || optopt == 'Q' )
                fprintf(stderr, "Option -%c is followed by a parameter.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            break;
    }
}

void mountLQ(char* args, Configs* config) {
    char* ptr = NULL;
    char* token  = strtok_r(args, " ", &ptr);

    link_quality_alg** alg = &config->discovery.d_args->lq_alg;

    char* name;
	if(strcmp(token, (name = "Random")) == 0  || strcmp(token, (name = "RandomQuality")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
            double prob = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
                double scalling = strtod(token, NULL);

    			*alg = RandomQuality(prob, scalling);
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "OLRS")) == 0  || strcmp(token, (name = "OLRSQuality")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
            double hyst_scaling = strtod(token, NULL);

            *alg = OLRSQuality(hyst_scaling);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else {
		printf("Unrecognized Broadcast Algorithm! \n");
		exit(-1);
	}
    strcpy(config->discovery.lq_alg_str, name);
}

void mountBroadcastAlgorithm(char* args, Configs* config) {

    strcpy(config->broadcast.r_alg, args);

    strcpy(config->broadcast.r_delay, "");
    strcpy(config->broadcast.r_policy, "");
    strcpy(config->broadcast.r_context, "");

    char* ptr = NULL;
    char* token  = strtok_r(args, " ", &ptr);

    if(token == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    BroadcastAlgorithm** bcast_alg = &config->broadcast.f_args->algorithm;

    destroyBroadcastAlgorithm(*bcast_alg);

	char* name;
	if(strcmp(token, (name = "Flooding")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);
			//printf(" %lu\n", max_delay);

			*bcast_alg = Flooding(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip1")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double p = strtod (token, NULL);

				*bcast_alg = Gossip1(t, p);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip1_hops")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double p = strtod (token, NULL);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					unsigned int k = strtol(token, NULL, 10);

					*bcast_alg = Gossip1_hops(t, p, k);
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
	} else if(strcmp(token, (name = "Gossip2")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double p1 = strtod (token, NULL);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					unsigned int k = strtol(token, NULL, 10);

					token = strtok_r(NULL, " ", &ptr);
					if(token != NULL) {
						double p2 = strtod (token, NULL);

						token = strtok_r(NULL, " ", &ptr);
						if(token != NULL) {
							unsigned int n = strtol(token, NULL, 10);

                            topology_discovery_args* d_args = config->discovery.d_args;
                            unsigned int window_size = config->discovery.window_size;

							*bcast_alg = Gossip2(t, p1, k, p2, n, window_size, d_args);
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
	} else if(strcmp(token, (name = "Gossip3")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t1 = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned long t2 = strtol(token, NULL, 10);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					double p = strtod (token, NULL);

					token = strtok_r(NULL, " ", &ptr);
					if(token != NULL) {
						unsigned int k = strtol(token, NULL, 10);

						token = strtok_r(NULL, " ", &ptr);
						if(token != NULL) {
							unsigned int m = strtol(token, NULL, 10);

							*bcast_alg = Gossip3(t1, t2, p, k, m);
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
	} else if(strcmp(token, (name = "Rapid")) == 0 || strcmp(token, (name = "RAPID")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double beta = strtod(token, NULL);

                topology_discovery_args* d_args = config->discovery.d_args;
                unsigned int window_size = config->discovery.window_size;

				*bcast_alg = RAPID(t, beta, window_size, d_args);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "EnhancedRapid")) == 0 || strcmp(token, (name = "EnhancedRAPID")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t1 = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned long t2 = strtol(token, NULL, 10);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					double beta = strtod(token, NULL);

                    topology_discovery_args* d_args = config->discovery.d_args;
                    unsigned int window_size = config->discovery.window_size;

					*bcast_alg = EnhancedRAPID(t1, t2, beta, window_size, d_args);
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
	} else if(strcmp(token, (name = "Counting")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned int c = strtol(token, NULL, 10);

				*bcast_alg = Counting(t, c);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "HopCountAided")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			*bcast_alg = HopCountAided(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA1")) == 0 || strcmp(token, (name = "CountingNABA")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned int c = strtol(token, NULL, 10);

                topology_discovery_args* d_args = config->discovery.d_args;
                unsigned int window_size = config->discovery.window_size;

				*bcast_alg = NABA1(t, c, window_size, d_args);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA2")) == 0 || strcmp(token, (name = "PbCountingNABA")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned int c1 = strtol(token, NULL, 10);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					unsigned int c2 = strtol(token, NULL, 10);

                    topology_discovery_args* d_args = config->discovery.d_args;
                    unsigned int window_size = config->discovery.window_size;

					*bcast_alg = NABA2(t, c1, c2, window_size, d_args);
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
	}
    else if(strcmp(token, (name = "NABA3")) == 0 || strcmp(token, (name = "Naba3")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            topology_discovery_args* d_args = config->discovery.d_args;
            unsigned int window_size = config->discovery.window_size;

            *bcast_alg = NABA3(t, window_size, d_args);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA4")) == 0 || strcmp(token, (name = "Naba4")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double min_critical_coverage = strtod(token, NULL);

                topology_discovery_args* d_args = config->discovery.d_args;
                unsigned int window_size = config->discovery.window_size;

				*bcast_alg = NABA4(t, min_critical_coverage, window_size, d_args);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA3e4")) == 0 || strcmp(token, (name = "Naba3e4")) == 0 || strcmp(token, (name = "CriticalNABA")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double min_critical_coverage = strtod(token, NULL);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					unsigned int np = strtol(token, NULL, 10);

                    topology_discovery_args* d_args = config->discovery.d_args;
                    unsigned int window_size = config->discovery.window_size;

					*bcast_alg = NABA3e4(t, min_critical_coverage, np, window_size, d_args);
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
	}
    else if(strcmp(token, (name = "SBA")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            topology_discovery_args* d_args = config->discovery.d_args;
            unsigned int window_size = config->discovery.window_size;

            *bcast_alg = SBA(t, window_size, d_args);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "LENWB")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            topology_discovery_args* d_args = config->discovery.d_args;
            unsigned int window_size = config->discovery.window_size;

            *bcast_alg = LENWB(t, window_size, d_args);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "MPR")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			double hyst_threshold_low = strtod(token, NULL);

                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
        			double hyst_threshold_high = strtod(token, NULL);

                    topology_discovery_args* d_args = config->discovery.d_args;
                    unsigned int window_size = config->discovery.window_size;

                    *bcast_alg = MPR(t, hyst_threshold_low, hyst_threshold_high, window_size, d_args);
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
	} else if(strcmp(token, (name = "AHBP")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			int ex = (int) strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned long t = strtol(token, NULL, 10);

                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
        			double hyst_threshold_low = strtod(token, NULL);

                    token = strtok_r(NULL, " ", &ptr);
            		if(token != NULL) {
            			double hyst_threshold_high = strtod(token, NULL);

                        token = strtok_r(NULL, " ", &ptr);
                		if(token != NULL) {
                			unsigned int route_max_len = (unsigned int)strtol(token, NULL, 10);

                            topology_discovery_args* d_args = config->discovery.d_args;
                            unsigned int window_size = config->discovery.window_size;

                            *bcast_alg = AHBP(ex, t, hyst_threshold_low, hyst_threshold_high, route_max_len, window_size, d_args);
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
	} else if(strcmp(token, (name = "DynamicProbability")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			double p_l = strtod(token, NULL);

                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
        			double p_u = strtod(token, NULL);

                    token = strtok_r(NULL, " ", &ptr);
            		if(token != NULL) {
            			double d = strtod(token, NULL);

                        token = strtok_r(NULL, " ", &ptr);
                		if(token != NULL) {
                			unsigned long t1 = strtol(token, NULL, 10);

                            token = strtok_r(NULL, " ", &ptr);
                    		if(token != NULL) {
                    			unsigned long t2 = strtol(token, NULL, 10);

                                *bcast_alg = DynamicProbability(p, p_l, p_u, d, t1, t2);
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
    } else if(strcmp(token, (name = "RADExtension")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long delta_t = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int c = strtol(token, NULL, 10);

                *bcast_alg = RADExtension(delta_t, c);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if(strcmp(token, (name = "HopCountAwareRADExtension")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long delta_t = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int c = strtol(token, NULL, 10);

                *bcast_alg = HopCountAwareRADExtension(delta_t, c);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else {
		printf("Unrecognized Broadcast Algorithm! \n");
		exit(-1);
	}
}

RetransmissionDelay* parseRetransmissionDelay(char* token, char* ptr, Configs* config) {

	char* name;
	if(strcmp(token, (name = "Random")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return RandomDelay(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "DensityNeigh")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return DensityNeighDelay(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "SBADelay")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return SBADelay(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "TwoPhaseRandomDelay")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t1 = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned long t2 = strtol(token, NULL, 10);

                return TwoPhaseRandomDelay(t1, t2);
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Null")) == 0 || strcmp(token, (name = "NullDelay")) == 0) {
        return NullDelay();
	} else if(strcmp(token, (name = "RADExtensionDelay")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long delta_t = strtol(token, NULL, 10);

            return RADExtensionDelay(delta_t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "HopCountAwareRADExtensionDelay")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long delta_t = strtol(token, NULL, 10);

            return HopCountAwareRADExtensionDelay(delta_t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else {
		exit(-1);
		printf("Unrecognized R. Delay! \n");
	}
}

void mountRetransmissionDelay(char* args, Configs* config) {

    bzero(config->broadcast.r_delay, STR_SIZE);
    strcpy(config->broadcast.r_delay, args);

    char* ptr = NULL;
    char* token  = strtok_r(args, " ", &ptr);

    if(token == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    BroadcastAlgorithm* bcast_alg = config->broadcast.f_args->algorithm;
    RetransmissionDelay* r_delay = parseRetransmissionDelay(token, ptr, config);
    RetransmissionDelay* old_delay = setRetransmissionDelay(bcast_alg, r_delay);
    if(old_delay) destroyRetransmissionDelay(old_delay, NULL);
}


RetransmissionPolicy* parseRetransmissionPolicy(char* token, char* ptr, Configs* config) {
	char* name;
	if(strcmp(token, (name = "True")) == 0 || strcmp(token, (name = "AlwaysTrue")) == 0 || strcmp(token, (name = "TruePolicy")) == 0 ) {
        return TruePolicy();
	} else if(strcmp(token, (name = "Probability")) == 0 || strcmp(token, (name = "ProbabilityPolicy")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p = strtod (token, NULL);

            return ProbabilityPolicy(p);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Count")) == 0 || strcmp(token, (name = "CountPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned int c = strtol(token, NULL, 10);

            return CountPolicy(c);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NeighborCounting")) == 0 || strcmp(token, (name = "NeighborCountingPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned int c = strtol(token, NULL, 10);

            return NeighborCountingPolicy(c);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "PbNeighCounting")) == 0 || strcmp(token, (name = "PbNeighCountingPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned int c1 = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int c2 = strtol(token, NULL, 10);

                return PbNeighCountingPolicy(c1, c2);
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "HorizonProbability")) == 0 || strcmp(token, (name = "HorizonProbabilityPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int k = strtol(token, NULL, 10);

                return HorizonProbabilityPolicy(p, k);
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip2")) == 0 || strcmp(token, (name = "Gossip2Policy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p1 = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int k = strtol(token, NULL, 10);

                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
        			double p2 = strtod(token, NULL);

                    token = strtok_r(NULL, " ", &ptr);
            		if(token != NULL) {
            			unsigned int n = strtol(token, NULL, 10);

                        return Gossip2Policy(p1, k, p2, n);
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
	} else if(strcmp(token, (name = "Rapid")) == 0 || strcmp(token, (name = "RapidPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double beta = strtod(token, NULL);

            return RapidPolicy(beta);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "EnhancedRapid")) == 0 || strcmp(token, (name = "EnhancedRapidPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double beta = strtod(token, NULL);

            return EnhancedRapidPolicy(beta);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
    } else if(strcmp(token, (name = "Gossip3")) == 0 || strcmp(token, (name = "Gossip3Policy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int k = strtol(token, NULL, 10);

                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
        			unsigned int m = strtol(token, NULL, 10);

                    return Gossip3Policy(p, k, m);
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
	} else if(strcmp(token, (name = "HopCountAided")) == 0 || strcmp(token, (name = "HopCountAidedPolicy")) == 0) {
        return HopCountAidedPolicy();
    } else if(strcmp(token, (name = "SBA")) == 0 || strcmp(token, (name = "SBAPolicy")) == 0) {
        return SBAPolicy();
    } else if(strcmp(token, (name = "LENWB")) == 0 || strcmp(token, (name = "LENWBPolicy")) == 0) {
        return LENWBPolicy();
    } else if(strcmp(token, (name = "DelegatedNeighbors")) == 0 || strcmp(token, (name = "DelegatedNeighborsPolicy")) == 0) {
        return DelegatedNeighborsPolicy();
    } else if(strcmp(token, (name = "CriticalNeigh")) == 0 || strcmp(token, (name = "CriticalNeighPolicy")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double min_critical_coverage = strtod(token, NULL);

            return CriticalNeighPolicy(min_critical_coverage);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if(strcmp(token, (name = "AHBP")) == 0 || strcmp(token, (name = "AHBPPolicy")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            int ex = strtol(token, NULL, 10);

            return AHBPPolicy(ex);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if(strcmp(token, (name = "DynamicProbability")) == 0 || strcmp(token, (name = "DynamicProbabilityPolicy")) == 0) {
        return DynamicProbabilityPolicy();
    } else {
        printf("Unrecognized Retransmission Policy! \n");
		exit(-1);
	}
}

void mountRetransmissionPolicy(char* args, Configs* config) {

    bzero(config->broadcast.r_policy, STR_SIZE);
    strcpy(config->broadcast.r_policy, args);

    char* ptr = NULL;
    char* token  = strtok_r(args, " ", &ptr);

    if(token == NULL) {
        fprintf(stderr, "No parameter passed");
        exit(-1);
    }

    BroadcastAlgorithm* bcast_alg = config->broadcast.f_args->algorithm;
    RetransmissionPolicy* r_policy = parseRetransmissionPolicy(token, ptr, config);
    RetransmissionPolicy* old_policy = setRetransmissionPolicy(bcast_alg, r_policy);
    if(old_policy) destroyRetransmissionPolicy(old_policy, NULL);
}


RetransmissionContext* parseRetransmissionContext(char* token, char* ptr, Configs* config) {
    const char* name;
	if(strcmp(token, (name = "Empty")) == 0 || strcmp(token, (name = "EmptyContext")) == 0) {
        return EmptyContext();
	} else if(strcmp(token, (name = "Hops")) == 0 || strcmp(token, (name = "HopsContext")) == 0) {
		return HopsContext();
	} else if(strcmp(token, (name = "Parents")) == 0 || strcmp(token, (name = "ParentsContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned int max_len = strtol(token, NULL, 10);

    		return ParentsContext(max_len);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "Neighbors")) == 0 || strcmp(token, (name = "NeighborsContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);

        topology_discovery_args* d_args = config->discovery.d_args;
        unsigned int window_size = config->discovery.window_size;

        return NeighborsContext(window_size, d_args);
	} else if(strcmp(token, (name = "MultiPointRelay")) == 0 || strcmp(token, (name = "MultiPointRelayContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double hyst_threshold_low = strtod(token, NULL);
            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                double hyst_threshold_high = strtod(token, NULL);

                topology_discovery_args* d_args = config->discovery.d_args;
                unsigned int window_size = config->discovery.window_size;
                RetransmissionContext* neighbors_context = NeighborsContext(window_size, d_args);

                return MultiPointRelayContext(neighbors_context, hyst_threshold_low, hyst_threshold_high);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "AHBP")) == 0 || strcmp(token, (name = "AHBPContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double hyst_threshold_low = strtod(token, NULL);
            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                double hyst_threshold_high = strtod(token, NULL);

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    unsigned int max_len = strtol(token, NULL, 10);

                    topology_discovery_args* d_args = config->discovery.d_args;
                    unsigned int window_size = config->discovery.window_size;
                    RetransmissionContext* neighbors_context = NeighborsContext(window_size, d_args);

                    RetransmissionContext* route_context = RouteContext(max_len);

                    return AHBPContext(neighbors_context, route_context, hyst_threshold_low, hyst_threshold_high);
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
	} else if(strcmp(token, (name = "Route")) == 0 || strcmp(token, (name = "RouteContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned int max_len = strtol(token, NULL, 10);

            return RouteContext(max_len);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "DynamicProbability")) == 0 || strcmp(token, (name = "DynamicProbabilityContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double p = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                double p_l = strtod(token, NULL);

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    double p_u = strtod(token, NULL);

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        double d = strtod(token, NULL);

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            unsigned long t = strtol(token, NULL, 10);

                            return DynamicProbabilityContext(p, p_l, p_u, d, t);
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
	} else if(strcmp(token, (name = "LabelNeighs")) == 0 || strcmp(token, (name = "LabelNeighsContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            const char* nc1 = "NeighborsContext";
            const char* nc2 = "Neighbors";
            if(strncmp(token, nc1, strlen(nc1)) == 0 || strncmp(token, nc2, strlen(nc2)) == 0) {
                RetransmissionContext* neighbors_context = parseRetransmissionContext(token, ptr, config);

                return LabelNeighsContext(neighbors_context);
            } else {
                printf("Parameter 2 of %s is invalid!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "HopCountAwareRADExtension")) == 0 || strcmp(token, (name = "HopCountAwareRADExtensionContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned long delta_t = strtol(token, NULL, 10);

            return HopCountAwareRADExtensionContext(delta_t);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "Monitor")) == 0 || strcmp(token, (name = "MonitorContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned long log_period = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                char aux[100];
                strcpy(aux, token);
                strrplc(aux, ':', ' ');

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    RetransmissionContext* child_context = parseRetransmissionContext(token, ptr, config);

                    return MonitorContext(log_period, aux, child_context);
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
	} else if(strcmp(token, (name = "Latency")) == 0 || strcmp(token, (name = "LatencyContext")) == 0) {
        return LatencyContext();
	} else {
        printf("Unrecognized Retransmission Context! \n");
		exit(-1);
	}
}

void mountRetransmissionContext(char* args, Configs* config) {

    bzero(config->broadcast.r_context, STR_SIZE);
    strcpy(config->broadcast.r_context, args);

    char* ptr = NULL;
    char* token  = strtok_r(args, " ", &ptr);

    if(token == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    BroadcastAlgorithm* bcast_alg = config->broadcast.f_args->algorithm;
    RetransmissionContext* r_context = parseRetransmissionContext(token, ptr, config);
    RetransmissionContext* old_context = setRetransmissionContext(bcast_alg, r_context);
    if(old_context) destroyRetransmissionContext(old_context, NULL);
}
