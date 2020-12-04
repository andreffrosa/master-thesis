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
