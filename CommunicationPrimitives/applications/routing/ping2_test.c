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
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <linux/limits.h>


#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Yggdrasil.h"

#include "../app_common.h"

#include "protocols/routing/framework/framework.h"
#include "protocols/broadcast/framework/framework.h"
#include "protocols/discovery/framework/framework.h"

#include "utility/my_math.h"
#include "utility/my_time.h"
#include "utility/my_sys.h"
#include "utility/my_string.h"

#include "utility/my_logger.h"

#define ID_TEMPLATE "66600666-1001-1001-1001-000000000001"

extern void (*log_flush_handler)();

typedef struct RoutingAppArgs_ {
    list* destinations;
    list* sources;

    bool loop_destinations;

    //uuid_t destination_id;
    //bool rcv_only;

    unsigned long exp_duration_s;
    //unsigned long max_pings;

    unsigned long ping_interval_ms;

    unsigned long pings_per_destination;

    unsigned long initial_grace_period_s;
    unsigned long final_grace_period_s;

    bool verbose;
} RoutingAppArgs;

typedef struct RoutingAppState_ {
    list* destinations;

    unsigned char* current_destination;

    bool rcv_only;

    unsigned int counter;
    unsigned int last_rep_counter;

    bool finished;
} RoutingAppState;

static RoutingAppArgs* default_routing_app_args();
static RoutingAppArgs* parse_routing_app_args(const char* file_path);

static RoutingAppState* init_app_state(RoutingAppArgs* app_args, unsigned char* myID);

static list* parse_int_list(char* value);
static void parse_node(unsigned char* id, unsigned int node_id);

static void uponSendTimer(RoutingAppArgs* app_args, RoutingAppState* app_state, unsigned char* send_timer_id, unsigned char* myID);

static void printDiscoveryStats(YggRequest* req);
static void printBroadcastStats(YggRequest* req);
static void printRoutingStats(YggRequest* req);

static void sendMessage(RoutingAppArgs* app_args, RoutingAppState* app_state);
static void rcvMessage(YggMessage* msg, RoutingAppArgs* app_args, RoutingAppState* app_state);

static void uponNotification(YggEvent* ev, RoutingAppArgs* app_args);

static void requestDiscoveryStats();
static void requestBroadcastStats();
static void requestRoutingStats();

static void setRoutingTimer(RoutingAppArgs* app_args, unsigned char* send_timer_id, bool isFirst/*, struct timespec* start_time*/);

#define APP_ID 400

#define APP_NAME "PING2 APP"

/*static*/ MyLogger* app_logger = NULL;

void my_log_flush_handler();

int main(int argc, char* argv[]) {

    srand(time(NULL));

    log_flush_handler = &my_log_flush_handler;

	// Process Args
    hash_table* args = parse_args(argc, argv);

    // Interface and Hostname
    char interface[20] = {0}, hostname[30] = {0};
    unparse_host(hostname, 20, interface, 30, args);

    // Logs
    //char destination[200] = "../experiments/output/routing/exp1-";
    //hash_table_insert(args, new_str("log"), new_str(strcat(destination, hostname))); // temp

    char* log_path = hash_table_find_value(args, "log");
    if(log_path) {
        //rmdir(log_path);
        //char cmd[100];
        //sprintf(cmd, "rm -r %s", log_path);
        //run_command(cmd, NULL, 0);

        // Create dir if does not exist
        struct stat st = {0};
        if (stat(log_path, &st) == -1) {
            if(mkdir(log_path, S_IRWXO) != 0) {
                fprintf(stderr, "Could not create dir %s!\n", log_path);
                exit(-1);
            } else {
                //printf("Creating dir %s ...\n", log_path);
            }
        }

        char file_path[PATH_MAX];
        build_path(file_path, log_path, "app.log");
        //app_logger = new_my_logger(fopen("../experiments/output/routing/test.txt", "w"), hostname);
        //app_logger = new_my_logger(stdout, hostname);

        FILE* f = freopen(file_path, "w", stdout);
        if(f) {
            printf("Creating log %s ...\n", file_path);
        } else {
            fprintf(stderr, "[ERROR] Could not create log file %s\n", file_path);
            exit(-1);
        }

        dup2(1, 2);  //redirects stderr to stdout below this line.

        app_logger = new_my_logger(stdout, hostname);

        build_path(file_path, log_path, "discovery.log");
        f = fopen(file_path, "w");
        if(f) {
            printf("Creating log %s ...\n", file_path);
        } else {
            fprintf(stderr, "[ERROR] Could not create log file %s\n", file_path);
            exit(-1);
        }
        discovery_logger = new_my_logger(f, hostname);

        build_path(file_path, log_path, "neighbors.log");
        f = fopen(file_path, "w");
        if(f) {
            printf("Creating log %s ...\n", file_path);
        } else {
            fprintf(stderr, "[ERROR] Could not create log file %s\n", file_path);
            exit(-1);
        }
        neighbors_logger = new_my_logger(f, hostname);

        build_path(file_path, log_path, "broadcast.log");
        f = fopen(file_path, "w");
        if(f) {
            printf("Creating log %s ...\n", file_path);
        } else {
            fprintf(stderr, "[ERROR] Could not create log file %s\n", file_path);
            exit(-1);
        }
        broadcast_logger = new_my_logger(f, hostname);

        build_path(file_path, log_path, "routing.log");
        f = fopen(file_path, "w");
        if(f) {
            printf("Creating log %s ...\n", file_path);
        } else {
            fprintf(stderr, "[ERROR] Could not create log file %s\n", file_path);
            exit(-1);
        }
        routing_logger = new_my_logger(f, hostname);

        build_path(file_path, log_path, "routing_table.log");
        f = fopen(file_path, "w");
        if(f) {
            printf("Creating log %s ...\n", file_path);
        } else {
            fprintf(stderr, "[ERROR] Could not create log file %s\n", file_path);
            exit(-1);
        }
        routing_table_logger = new_my_logger(f, hostname);
    } else {
        printf("Using stdout as logger ...\n");
        app_logger = new_my_logger(stdout, hostname);
        discovery_logger = app_logger;
        neighbors_logger = app_logger;
        broadcast_logger = app_logger;
        routing_logger = app_logger;
        routing_table_logger = app_logger;
    }

    // Sleep
    char* sleep_ms = hash_table_find_value(args, "sleep");
    if(sleep_ms) {
        unsigned long t_ms = randomProb() * strtol(sleep_ms, NULL, 10);

        char str[30];
        sprintf(str, "%lu ms", t_ms);
        my_logger_write(app_logger, APP_NAME, "SLEEP", str);

        usleep(t_ms*1000);
    } else {
        my_logger_write(app_logger, APP_NAME, "SLEEP", "0");
    }

    // Network
    NetworkConfig* ntconf = defineNetworkConfig2(interface, "AdHoc", 2462, 3, 1, "pis", YGG_filter);

    // Initialize ygg_runtime
    ygg_runtime_init_2(ntconf, hostname);

    uuid_t myID;
    getmyId(myID);

    // Overlay
    char* overlay_path = hash_table_find_value(args, "overlay");
	if(overlay_path) {
        topology_manager_args* t_args = load_overlay(overlay_path, hostname);
        registerYggProtocol(PROTO_TOPOLOGY_MANAGER, topologyManager_init, t_args);
		topology_manager_args_destroy(t_args);
	}

    // Discovery Framework
    char* discovery_args_file = hash_table_find_value(args, "discovery");
    if(discovery_args_file) {
        discovery_framework_args* discovery_args = load_discovery_framework_args(discovery_args_file);

    	registerProtocol(DISCOVERY_FRAMEWORK_PROTO_ID, &discovery_framework_init, (void*) discovery_args);
    }

    // Broadcast Framework
    broadcast_framework_args* broadcast_args = NULL;
    char* broadcast_args_file = hash_table_find_value(args, "broadcast");
    if(broadcast_args_file) {
        broadcast_args = load_broadcast_framework_args(broadcast_args_file);

        registerProtocol(BROADCAST_FRAMEWORK_PROTO_ID, &broadcast_framework_init, (void*) broadcast_args);
    }

    // Routing Framework
    routing_framework_args* routing_args = NULL;
    char* routing_args_file = hash_table_find_value(args, "routing");
    if(routing_args_file) {
        routing_args = load_routing_framework_args(routing_args_file);
    } else {
        routing_args = default_routing_framework_args();
    }
    registerProtocol(ROUTING_FRAMEWORK_PROTO_ID, &routing_framework_init, (void*) routing_args);

    // Routing App
    RoutingAppArgs* app_args = NULL;
    char* app_args_file = hash_table_find_value(args, "app");
    if(app_args_file) {
        app_args = parse_routing_app_args(app_args_file);
    } else {
        app_args = default_routing_app_args();
    }
    // TODO: print app_args

    hash_table_delete(args);
    args = NULL;

    RoutingAppState* app_state = init_app_state(app_args, myID);

    // Register this app
    app_def* myApp = create_application_definition(APP_ID, APP_NAME);

    /*
    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBOR_FOUND);
    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBOR_UPDATE);
    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBOR_LOST);
    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBORHOOD_UPDATE);
    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, IN_TRAFFIC);
    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, OUT_TRAFFIC);
*/

    queue_t* inBox = registerApp(myApp);

	// Start ygg_runtime
	ygg_runtime_start();

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Start App

    char str[100];
    sprintf(str, "%s starting experience with duration %lu + %lu + %lu s\n", hostname, app_args->initial_grace_period_s, app_args->exp_duration_s, app_args->final_grace_period_s);
    my_logger_write(app_logger, APP_NAME, "INIT", str);

    struct timespec start_time = {0};
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    struct timespec t = {0};

	// Set end of experiment
	uuid_t end_exp;
	genUUID(end_exp);
	milli_to_timespec(&t, (app_args->exp_duration_s + app_args->initial_grace_period_s)*1000);
    SetTimer(&t, end_exp, APP_ID, 0);

    uuid_t kill_exp;
	genUUID(kill_exp);

    // Set periodic timer
    uuid_t send_timer_id;
    genUUID(send_timer_id);
    if(!app_state->rcv_only){
        setRoutingTimer(app_args, send_timer_id, true/*, &start_time*/);
    }

	//unsigned int counter = 0, last_rep_counter = 0;
    //bool finished = false;

	queue_t_elem elem = {0};
	while(1) {
		queue_pop(inBox, &elem);

		switch(elem.type) {
		case YGG_TIMER:

			if( uuid_compare(elem.data.timer.id, end_exp ) == 0 ) {
                if(!app_state->finished) {
                    app_state->finished = true;
                    my_logger_write(app_logger, APP_NAME, "MAIN LOOP", "END.");
                    my_logger_write(discovery_logger, DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "END.");
                    my_logger_write(neighbors_logger, DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "END.");
                    my_logger_write(broadcast_logger, BROADCAST_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "END.");
                    my_logger_write(routing_logger, ROUTING_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "END.");
                    my_logger_write(routing_table_logger, ROUTING_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "END.");

                    /*my_logger_flush(app_logger);
                    my_logger_flush(discovery_logger);
                    my_logger_flush(neighbors_logger);*/

                    milli_to_timespec(&t, app_args->final_grace_period_s*1000);
                    SetTimer(&t, end_exp, APP_ID, 0);
                } else {
                    requestDiscoveryStats();
                    requestBroadcastStats();
                    requestRoutingStats();
                    milli_to_timespec(&t, 500);
                    SetTimer(&t, kill_exp, APP_ID, 0);
                }
			} else if( uuid_compare(elem.data.timer.id, send_timer_id) == 0 ) {
                uponSendTimer(app_args, app_state, send_timer_id, myID);
			}  else if( uuid_compare(elem.data.timer.id, kill_exp) == 0 ) {

                my_log_flush_handler();

                //return 0;
			}

            /*if(finished) {
                deliverRequest(&framework_stats_req);
            }*/

			//deliverRequest(&discovery_req);
			//deliverRequest(&framework_stats_req);
			//printNeighbours();

			break;
		case YGG_REQUEST:
            ;YggRequest* req = &elem.data.request;
			if( req->proto_dest == APP_ID ) {

				if( req->proto_origin == DISCOVERY_FRAMEWORK_PROTO_ID ) {
					if( req->request == REPLY && req->request_type == REQ_DISCOVERY_FRAMEWORK_STATS) {
						printDiscoveryStats(req);
					}
				} else if ( req->proto_origin == BROADCAST_FRAMEWORK_PROTO_ID ) {
					if( req->request == REPLY && req->request_type == REQ_BROADCAST_FRAMEWORK_STATS) {
						printBroadcastStats(req);
					}
				} else if ( req->proto_origin == ROUTING_FRAMEWORK_PROTO_ID ) {
					if( req->request == REPLY && req->request_type == REQ_ROUTING_FRAMEWORK_STATS) {
						printRoutingStats(req);
					}
				}
			}
			break;
		case YGG_MESSAGE:
			rcvMessage(&elem.data.msg, app_args, app_state);
            /*if(finished) {
                deliverRequest(&framework_stats_req);
                deliverRequest(&discovery_stats_req);
            }*/
			break;
		case YGG_EVENT:
			uponNotification(&elem.data.event, app_args);
			break;
		default:
			my_logger_write(app_logger, APP_NAME, "MAIN LOOP", "Received wierd thing in my queue.");
		}

        // Release memory of elem payload
		free_elem_payload(&elem);
	}

	return 0;
}

static void sendMessage(RoutingAppArgs* app_args, RoutingAppState* app_state) {

    if( app_state->last_rep_counter < app_state->counter-1 ) {
        if(app_args->verbose) {
            char str[100];
            sprintf(str, "[%u]", app_state->counter-1);

            my_logger_write(app_logger, APP_NAME, "TIMEOUT", str);
        }
    }

    /*char dest_str[UUID_STR_LEN];
    uuid_unparse(app_state->current_destination, dest_str);*/

    char payload[100];
	struct timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);

    sprintf(payload, "PING [%u] [%lu:%lu]", /*dest_str,*/ app_state->counter, current_time.tv_sec, current_time.tv_nsec);

    //my_logger_write(app_logger, ); // TODO

    assert(app_state->current_destination);

    RouteMessage(app_state->current_destination, APP_ID, -1, false, (unsigned char*)payload, strlen(payload)+1);
}

static void rcvMessage(YggMessage* msg, RoutingAppArgs* app_args, RoutingAppState* app_state) {
    assert(msg->dataLen > 0 && msg->data != NULL);

    void* ptr = NULL;

    unsigned short src_proto = 0;
    ptr = YggMessage_readPayload(msg, ptr, &src_proto, sizeof(src_proto));
    assert(src_proto == ROUTING_FRAMEWORK_PROTO_ID);

    unsigned short payload_size = 0;
    ptr = YggMessage_readPayload(msg, ptr, &payload_size, sizeof(payload_size));

    char m[payload_size];
    ptr = YggMessage_readPayload(msg, ptr, m, payload_size);

    uuid_t source_id;
    ptr = YggMessage_readPayload(msg, ptr, source_id, sizeof(uuid_t));

    // Unparse
    char type[5] = {0};
    //char dest_str[UUID_STR_LEN];
    //uuid_t dest = {0};
    unsigned int counter = 0;
    struct timespec time = {0};

    //printf("%s\n", m);
    //fflush(stdout);

    int ret = sscanf(m, "%s [%u] [%lu:%lu]", type, &counter, &time.tv_sec, &time.tv_nsec);
    assert(ret == 4);

    //uuid_parse(dest, dest_str);

    if(strcmp(type, "PING") == 0) {
        if(app_args->verbose) {
            char src_str[UUID_STR_LEN];
            uuid_unparse(source_id, src_str);

            char str[100];
            sprintf(str, "[%u] from %s : s=%d bytes", counter, src_str, payload_size);
            my_logger_write(app_logger, APP_NAME, "REQ", str);
        }

        // Send Reply

        m[1] = 'O';
        RouteMessage(source_id, APP_ID, -1, false, (unsigned char*)m, strlen(m)+1);
    } else if(strcmp(type, "PONG") == 0) {

        app_state->last_rep_counter = counter;

        if(app_args->verbose) {
            struct timespec current_time;
            clock_gettime(CLOCK_MONOTONIC, &current_time);

            subtract_timespec(&time, &current_time, &time);
            unsigned long t_ms = timespec_to_milli(&time);

            char dest_str[UUID_STR_LEN];
            uuid_unparse(source_id, dest_str);

            char str[100];
            sprintf(str, "[%u] from %s : t=%lu ms s=%d bytes", counter, dest_str, t_ms, payload_size);

            my_logger_write(app_logger, APP_NAME, "REP", str);
        }
    } else {
        printf("Unknown message type!\n");
        assert(false);
    }
}

static void requestDiscoveryStats() {
    YggRequest req;
	YggRequest_init(&req, APP_ID, DISCOVERY_FRAMEWORK_PROTO_ID, REQUEST, REQ_DISCOVERY_FRAMEWORK_STATS);
    deliverRequest(&req);
}

static void requestBroadcastStats() {
    YggRequest req;
	YggRequest_init(&req, APP_ID, BROADCAST_FRAMEWORK_PROTO_ID, REQUEST, REQ_BROADCAST_FRAMEWORK_STATS);
    deliverRequest(&req);
}

static void requestRoutingStats() {
    YggRequest req;
	YggRequest_init(&req, APP_ID, ROUTING_FRAMEWORK_PROTO_ID, REQUEST, REQ_ROUTING_FRAMEWORK_STATS);
    deliverRequest(&req);
}

static void printBroadcastStats(YggRequest* req) {
	broadcast_stats stats;
	YggRequest_readPayload(req, NULL, &stats, sizeof(broadcast_stats));

	char m[1000];
	sprintf(m, "Broadcast: %lu \t Received: %lu \t Transmitted: %lu \t Not_Transmitted: %lu \t Delivered: %lu",
            stats.messages_bcasted,
            stats.messages_received,
            stats.messages_transmitted,
			stats.messages_not_transmitted,
            stats.messages_delivered);

	my_logger_write(app_logger, APP_NAME, "BROADCAST STATS", m);
}

static void printDiscoveryStats(YggRequest* req) {
	discovery_stats stats;
	YggRequest_readPayload(req, NULL, &stats, sizeof(discovery_stats));

	char str[1000];
	sprintf(str, "Disc. Messages: %lu \t Piggybacked Hellos: %lu \t Lost Neighs: %lu \t New Neighs: %lu",
            stats.discovery_messages,
            stats.piggybacked_hellos,
            stats.lost_neighbors,
            stats.new_neighbors);

	my_logger_write(app_logger, APP_NAME, "DISCOVERY STATS", str);
}

static void printRoutingStats(YggRequest* req) {
	routing_stats stats;
	YggRequest_readPayload(req, NULL, &stats, sizeof(routing_stats));

	char m[1000];
	sprintf(m, "Sent: %lu \t Delivered: %lu \t Forwarded: %lu \t Not_Forwarded: %lu \t Received: %lu \t Ctrl_sent: %lu \t Ctrl_rcv: %lu",
            stats.messages_requested,
            stats.messages_delivered,
            stats.messages_forwarded,
			stats.messages_not_forwarded,
            stats.messages_received,
            stats.control_sent,
            stats.control_received);

	my_logger_write(app_logger, APP_NAME, "ROUTING STATS", m);
}


static void uponNotification(YggEvent* ev, RoutingAppArgs* app_args) {

	/*
	if( ev->proto_dest == APP_ID ) {
        if(ev->proto_origin == TOPOLOGY_DISCOVERY_PROTO_ID) {
            if(ev->notification_id == NEIGHBORHOOD_UPDATE) {
                char* an_str;
                printAnnounce(ev->payload, ev->length, -1, &an_str);
                char str[strlen(an_str)+2];
                sprintf(str, "\n%s", an_str);
                my_logger_write(app_logger, APP_NAME, "DISCOVERY", str);
                free(an_str);
            }
        }
	} else {
		char s[100];
		sprintf(s, "Received notification from protocol %d meant for protocol %d", ev->proto_origin, ev->proto_dest);
		my_logger_write(app_logger, APP_NAME, "NOTIFICATION", s);
	}
*/



}


static void setRoutingTimer(RoutingAppArgs* app_args, unsigned char* send_timer_id, bool isFirst/*, struct timespec* start_time*/) {

    /*struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    subtract_timespec(&current_time, &current_time, start_time);
    unsigned long elapsed_ms = timespec_to_milli(&current_time);

    unsigned long t = getNextDelay("Periodic 1.0 1", elapsed_ms);*/

    unsigned long t = 0;

    if(isFirst) {
        t = app_args->initial_grace_period_s*1000;
    } else {
        t = app_args->ping_interval_ms;
    }

    struct timespec t_;
    milli_to_timespec(&t_, t);
    SetTimer(&t_, send_timer_id, APP_ID, 0);
}

static RoutingAppArgs* default_routing_app_args() {
    RoutingAppArgs* app_args = malloc(sizeof(RoutingAppArgs));

    uuid_t d;

    list* l = list_init();
    parse_node(d, 2);
    list_add_item_to_tail(l, new_id(d));
    app_args->destinations = l;

    l = list_init();
    parse_node(d, 1);
    list_add_item_to_tail(l, new_id(d));
    app_args->sources = l;

    //strcpy(app_args->routing_type, "Exponential Constant 2.0");
    app_args->initial_grace_period_s = 10;
    app_args->final_grace_period_s = 30;
    app_args->pings_per_destination = 10;
    //app_args->max_pings = ULONG_MAX; // infinite
    app_args->exp_duration_s = 5*60; // 5 min
    //app_args->rcv_only = false;
    app_args->ping_interval_ms = 1000;
    app_args->verbose = true;

    app_args->loop_destinations = false;

    return app_args;
}

static RoutingAppArgs* parse_routing_app_args(const char* file_path) {
    list* order = list_init();
    hash_table* configs = parse_configs_order(file_path, &order);

    if(configs == NULL) {
        char str[100];
        sprintf(str, "Config file %s not found!", file_path);
        my_logger_write(app_logger, APP_NAME, "ARG ERROR", str);
        ygg_logflush();

        exit(-1);
    }

    RoutingAppArgs* app_args = default_routing_app_args();

    for(list_item* it = order->head; it; it = it->next) {
        char* key = (char*)it->data;
        char* value = (char*)hash_table_find_value(configs, key);

        if( value != NULL ) {
            /*if( strcmp(key, "routing_type") == 0 ) {
                strcpy(app_args->routing_type, value);
            } else*/ if( strcmp(key, "destinations") == 0 ) {
                list_delete(app_args->destinations);
                app_args->destinations = parse_int_list(value);
            } else if( strcmp(key, "initial_grace_period_s") == 0 ) {
                app_args->initial_grace_period_s = (unsigned long) strtol(value, NULL, 10);
            } else if( strcmp(key, "final_grace_period_s") == 0 ) {
                app_args->final_grace_period_s = (unsigned long) strtol(value, NULL, 10);
            } else if( strcmp(key, "pings_per_destination") == 0 ) {
                bool is_infinite = (strcmp(value, "infinite") == 0 || strcmp(value, "infinity") == 0 || strcmp(value, "inf") == 0);
                app_args->pings_per_destination = is_infinite ? ULONG_MAX : (unsigned int) strtol(value, NULL, 10);
            } else if( strcmp(key, "exp_duration_s") == 0 ) {
                app_args->exp_duration_s = (unsigned long) strtol(value, NULL, 10);
            } else if( strcmp(key, "ping_interval_ms") == 0 ) {
                app_args->ping_interval_ms = (unsigned long) strtol(value, NULL, 10);
            } else if( strcmp(key, "sources") == 0 ) {
                list_delete(app_args->sources);
                app_args->sources = parse_int_list(value);
                //app_args->rcv_only = parse_bool(value);
            } else if( strcmp(key, "verbose") == 0 ) {
                app_args->verbose = parse_bool(value);
            } else if( strcmp(key, "loop_destinations") == 0 ) {
                app_args->loop_destinations = parse_bool(value);
            } else {
                char str[50];
                sprintf(str, "Unknown Config %s = %s", key, value);
                my_logger_write(app_logger, APP_NAME, "ARG ERROR", str);
            }
        } else {
            char str[50];
            sprintf(str, "Empty Config %s", key);
            my_logger_write(app_logger, APP_NAME, "ARG ERROR", str);
        }
    }

    // Clean
    hash_table_delete(configs);
    list_delete(order);

    return app_args;
}

static void parse_node(unsigned char* id, unsigned int node_id) {
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

static list* parse_int_list(char* value) {
    //uuid_parse(value, app_args->destination_id);

    int len = strlen(value);
    char str[len+1];
    memcpy(str, value, len+1);
    str[0] = ' ';
    str[len] = ' ';

    //printf("parse_int_list = %s\n", str);

    list* l = list_init();

    char* ptr = NULL;
    char* token = NULL;
    char* x = str;

    while( (token  = strtok_r(x, " ", &ptr))) {
        x = NULL;

        int n = strtol(token, NULL, 10);

        if(n > 0) {
            uuid_t u;
            parse_node(u, n);

            /*char id_str[UUID_STR_LEN];
            uuid_unparse(u, id_str);
            printf("%d %s\n", n, id_str);*/

            list_add_item_to_tail(l, new_id(u));
        }
    }

    return l;
}

static void uponSendTimer(RoutingAppArgs* app_args, RoutingAppState* app_state, unsigned char* send_timer_id, unsigned char* myID) {

    /*if(first) {
        printf("starting\n");
    }*/

    if (!app_state->rcv_only && !app_state->finished) {

        if( app_state->counter % app_args->pings_per_destination == 0 ) {
            if(app_state->current_destination) {
                free(app_state->current_destination);
            }

            app_state->current_destination = list_remove_head(app_state->destinations);

            /*if(app_state->current_destination) {
                char str[UUID_STR_LEN];
                uuid_unparse(app_state->current_destination, str);
                printf("app_state->current_destination = %s (%d remaining)\n", str, app_state->destinations->size);
            } else {
                printf("app_state->current_destination = NULL (%d remaining)\n", app_state->destinations->size);
            }*/

            if(app_state->current_destination == NULL && app_args->loop_destinations) {
                list* l = list_clone(app_args->destinations, sizeof(uuid_t));
                void* aux = list_remove_item(l, &equalID, myID);
                if(aux) {
                    free(aux);
                }
                list_shuffle(l, 3);
                list_delete(app_state->destinations);
                app_state->destinations = l;

                app_state->current_destination = list_remove_head(app_state->destinations);
            }
        }

        if(app_state->current_destination) {
            app_state->counter++;
            sendMessage(app_args, app_state);
            setRoutingTimer(app_args, send_timer_id, false/*, &start_time*/);
        }
    }
}


static RoutingAppState* init_app_state(RoutingAppArgs* app_args, unsigned char* myID) {
    RoutingAppState* app_state = malloc(sizeof(RoutingAppState));

    list* l = list_clone(app_args->destinations, sizeof(uuid_t));
    void* aux = list_remove_item(l, &equalID, myID);
    if(aux) {
        free(aux);
    }
    list_shuffle(l, 3);
    app_state->destinations = l;

    app_state->current_destination = NULL;

    app_state->rcv_only = list_find_item(app_args->sources, &equalID, myID) == NULL;

    app_state->counter = 0;
    app_state->last_rep_counter = 0;
    app_state->finished = false;

    /*
    printf("destinations: ");
    for(list_item* it = l->head; it; it = it->next) {
        char str[UUID_STR_LEN];
        uuid_unparse(it->data, str);
        printf("%s ", str);
    }
    printf("\n");

    printf("rcv_only = %s\n", (app_state->rcv_only?"T":"F"));
    */

    return app_state;
}

void my_log_flush_handler() {
    //printf("my_log_flush_handler\n");

    my_logger_write(app_logger, APP_NAME, "MAIN LOOP", "KILL");
    my_logger_write(discovery_logger, DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "KILL");
    my_logger_write(neighbors_logger, DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "KILL");
    my_logger_write(broadcast_logger, BROADCAST_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "KILL");
    my_logger_write(routing_logger, ROUTING_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "KILL");
    my_logger_write(routing_table_logger, ROUTING_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "KILL");

    my_logger_flush(app_logger);
    my_logger_flush(discovery_logger);
    my_logger_flush(neighbors_logger);
    my_logger_flush(broadcast_logger);
    my_logger_flush(routing_logger);
    my_logger_flush(routing_table_logger);
}
