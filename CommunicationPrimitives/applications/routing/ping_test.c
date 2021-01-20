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

typedef struct RoutingAppArgs_ {
    uuid_t destination_id;
    bool rcv_only;

    unsigned long exp_duration_s;
    unsigned long max_pings;

    unsigned long initial_grace_period_s;
    unsigned long final_grace_period_s;

    bool verbose;
} RoutingAppArgs;

static RoutingAppArgs* default_routing_app_args();
static RoutingAppArgs* parse_routing_app_args(const char* file_path);

static void printDiscoveryStats(YggRequest* req);
static void printBcastStats(YggRequest* req);

static void sendMessage(unsigned int last_rep_counter, unsigned int counter, RoutingAppArgs* app_args);
static void rcvMessage(YggMessage* msg, unsigned int* last_rep_counter, RoutingAppArgs* app_args);

static void uponNotification(YggEvent* ev, RoutingAppArgs* app_args);

static void requestDiscoveryStats();
static void requestBroadcastStats();

static void setRoutingTimer(RoutingAppArgs* app_args, unsigned char* bcast_timer_id, bool isFirst, struct timespec* start_time);

#define APP_ID 400
#define APP_NAME "PING APP"

int main(int argc, char* argv[]) {

	// Process Args
    hash_table* args = parse_args(argc, argv);

    // Interface and Hostname
    char interface[20] = {0}, hostname[30] = {0};
    unparse_host(hostname, 20, interface, 30, args);

    // Network
    NetworkConfig* ntconf = defineNetworkConfig2(interface, "AdHoc", 2462, 3, 1, "pis", YGG_filter);

    // Initialize ygg_runtime
    ygg_runtime_init_2(ntconf, hostname);

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

    uuid_t myID;
    getmyId(myID);

    char str[100];
    sprintf(str, "%s starting experience with duration %lu + %lu + %lu s\n", hostname, app_args->initial_grace_period_s, app_args->exp_duration_s, app_args->final_grace_period_s);
    ygg_log(APP_NAME, "INIT", str);

    struct timespec start_time = {0};
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    struct timespec t = {0};

	// Set end of experiment
	uuid_t end_exp;
	genUUID(end_exp);
	milli_to_timespec(&t, (app_args->exp_duration_s + app_args->initial_grace_period_s)*1000);
    SetTimer(&t, end_exp, APP_ID, 0);

    // Set periodic timer
    uuid_t bcast_timer_id;
    genUUID(bcast_timer_id);
    if(!app_args->rcv_only){
        setRoutingTimer(app_args, bcast_timer_id, true, &start_time);
    }

	unsigned int counter = 0, last_rep_counter = 0;
    bool finished = false;

	queue_t_elem elem = {0};
	while(1) {
		queue_pop(inBox, &elem);

		switch(elem.type) {
		case YGG_TIMER:

			if( uuid_compare(elem.data.timer.id, end_exp ) == 0 ) {
                if(!finished) {
                    finished = true;
                    ygg_log(APP_NAME, "MAIN LOOP", "END.");

                    milli_to_timespec(&t, app_args->final_grace_period_s*1000);
                    SetTimer(&t, end_exp, APP_ID, 0);
                } else {
                    requestDiscoveryStats();
                    requestBroadcastStats();
                }
			} else if( uuid_compare(elem.data.timer.id, bcast_timer_id ) == 0 ) {

				if (!app_args->rcv_only && !finished) {

                    if( uuid_compare(myID, app_args->destination_id) != 0 ) {
                        sendMessage(last_rep_counter, ++counter, app_args);
                    }

                    if( counter < app_args->max_pings || app_args->max_pings == ULONG_MAX ) {
						setRoutingTimer(app_args, bcast_timer_id, false, &start_time);
					}
				}
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
						printBcastStats(req);
					}
				}
			}
			break;
		case YGG_MESSAGE:
			rcvMessage(&elem.data.msg, &last_rep_counter, app_args);
            /*if(finished) {
                deliverRequest(&framework_stats_req);
                deliverRequest(&discovery_stats_req);
            }*/
			break;
		case YGG_EVENT:
			uponNotification(&elem.data.event, app_args);
			break;
		default:
			ygg_log(APP_NAME, "MAIN LOOP", "Received wierd thing in my queue.");
		}

        // Release memory of elem payload
		free_elem_payload(&elem);
	}

	return 0;
}

static void sendMessage(unsigned int last_rep_counter, unsigned int counter, RoutingAppArgs* app_args) {

    if( last_rep_counter < counter-1 ) {
        if(app_args->verbose) {
            char str[100];
            sprintf(str, "[%u]", counter-1);

            ygg_log(APP_NAME, "TIMEOUT", str);
        }
    }

    char payload[100];
	struct timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);

    sprintf(payload, "REQ [%u] %lu %lu", counter, current_time.tv_sec, current_time.tv_nsec);

    RouteMessage(app_args->destination_id, APP_ID, -1, false, (unsigned char*)payload, strlen(payload)+1);
}

static void rcvMessage(YggMessage* msg, unsigned int* last_rep_counter, RoutingAppArgs* app_args) {
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
    char type[4] = {0};
    unsigned int counter = 0;
    struct timespec time = {0};

    printf("%s\n", m);
    fflush(stdout);

    int ret = sscanf(m, "%s [%u] %lu %lu", type, &counter, &time.tv_sec, &time.tv_nsec);
    assert(ret == 4);

    if(strcmp(type, "REQ") == 0) {
        m[2] = 'P';

        //printf("Received REQ, replying with: %s\n", m);
        //fflush(stdout);

        RouteMessage(source_id, APP_ID, -1, false, (unsigned char*)m, strlen(m)+1);
    } else {

        *last_rep_counter = counter;

        if(app_args->verbose) {
            struct timespec current_time;
            clock_gettime(CLOCK_MONOTONIC, &current_time);

            subtract_timespec(&time, &current_time, &time);
            unsigned long t_ms = timespec_to_milli(&time);

            char str[100];
            sprintf(str, "[%u] t=%lu ms", counter, t_ms);

            ygg_log(APP_NAME, "RECEIVED", str);
        }
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

static void printBcastStats(YggRequest* req) {
	broadcast_stats stats;
	YggRequest_readPayload(req, NULL, &stats, sizeof(broadcast_stats));

	char m[1000];
	sprintf(m, "Broadcast: %lu \t Received: %lu \t Transmitted: %lu \t Not_Transmitted: %lu \t Delivered: %lu",
            stats.messages_bcasted,
            stats.messages_received,
            stats.messages_transmitted,
			stats.messages_not_transmitted,
            stats.messages_delivered);

	ygg_log(APP_NAME, "BROADCAST STATS", m);
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

	ygg_log(APP_NAME, "DISCOVERY STATS", str);
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
                ygg_log(APP_NAME, "DISCOVERY", str);
                free(an_str);
            }
        }
	} else {
		char s[100];
		sprintf(s, "Received notification from protocol %d meant for protocol %d", ev->proto_origin, ev->proto_dest);
		ygg_log(APP_NAME, "NOTIFICATION", s);
	}
*/



}


static void setRoutingTimer(RoutingAppArgs* app_args, unsigned char* bcast_timer_id, bool isFirst, struct timespec* start_time) {

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    subtract_timespec(&current_time, &current_time, start_time);
    unsigned long elapsed_ms = timespec_to_milli(&current_time);

    unsigned long t = getNextDelay("Periodic 1.0 1", elapsed_ms);

    if(isFirst) {
        t += app_args->initial_grace_period_s*1000;
    }

    /*
    #ifdef DEBUG_BROADCAST_TEST
    char str[100];
    sprintf(str, "routing_timer = %lu\n", t);
    ygg_log(APP_NAME, "BROADCAST TIMER", str);
    #endif
    */

    struct timespec t_;
    milli_to_timespec(&t_, t);
    SetTimer(&t_, bcast_timer_id, APP_ID, 0);
}

static RoutingAppArgs* default_routing_app_args() {
    RoutingAppArgs* app_args = malloc(sizeof(RoutingAppArgs));

    uuid_parse("66600666-1001-1001-1001-000000000001", app_args->destination_id);

    //strcpy(app_args->routing_type, "Exponential Constant 2.0");
    app_args->initial_grace_period_s = 10;
    app_args->final_grace_period_s = 30;
    app_args->max_pings = ULONG_MAX; // infinite
    app_args->exp_duration_s = 5*60; // 5 min
    app_args->rcv_only = false;
    app_args->verbose = true;

    return app_args;
}

static RoutingAppArgs* parse_routing_app_args(const char* file_path) {
    list* order = list_init();
    hash_table* configs = parse_configs_order(file_path, &order);

    if(configs == NULL) {
        char str[100];
        sprintf(str, "Config file %s not found!", file_path);
        ygg_log(APP_NAME, "ARG ERROR", str);
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
            } else*/ if( strcmp(key, "destination_id") == 0 ) {
                uuid_parse(value, app_args->destination_id);
            } else if( strcmp(key, "initial_grace_period_s") == 0 ) {
                app_args->initial_grace_period_s = (unsigned long) strtol(value, NULL, 10);
            } else if( strcmp(key, "final_grace_period_s") == 0 ) {
                app_args->final_grace_period_s = (unsigned long) strtol(value, NULL, 10);
            } else if( strcmp(key, "max_pings") == 0 ) {
                bool is_infinite = (strcmp(value, "infinite") == 0 || strcmp(value, "infinity") == 0 || strcmp(value, "inf") == 0);
                app_args->max_pings = is_infinite ? ULONG_MAX : (unsigned int) strtol(value, NULL, 10);
            } else if( strcmp(key, "exp_duration_s") == 0 ) {
                app_args->exp_duration_s = (unsigned long) strtol(value, NULL, 10);
            } else if( strcmp(key, "rcv_only") == 0 ) {
                app_args->rcv_only = parse_bool(value);
            } else if( strcmp(key, "verbose") == 0 ) {
                app_args->verbose = parse_bool(value);
            } else {
                char str[50];
                sprintf(str, "Unknown Config %s = %s", key, value);
                ygg_log(APP_NAME, "ARG ERROR", str);
            }
        } else {
            char str[50];
            sprintf(str, "Empty Config %s", key);
            ygg_log(APP_NAME, "ARG ERROR", str);
        }
    }

    // Clean
    hash_table_delete(configs);
    list_delete(order);

    return app_args;
}
