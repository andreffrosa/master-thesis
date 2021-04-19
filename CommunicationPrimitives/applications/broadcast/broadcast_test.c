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

#include "protocols/broadcast/framework/framework.h"
#include "protocols/discovery/framework/framework.h"

#include "utility/my_math.h"
#include "utility/my_time.h"
#include "utility/my_sys.h"
#include "utility/my_string.h"

typedef struct BroadcastAppArgs_ {
    char broadcast_type[100];
    //struct timespec start_time;
    unsigned long initial_grace_period_s;
    unsigned long final_grace_period_s;

    //unsigned long initial_timer_ms;
    //unsigned long periodic_timer_ms;
    unsigned long max_broadcasts;
    unsigned long exp_duration_s;
    unsigned char rcv_only;
    //double bcast_prob;
    //bool use_overlay;
    //char overlay_path[PATH_MAX];
    //char interface_name[100];
    //char hostname[100];
    bool verbose;
} BroadcastAppArgs;

static BroadcastAppArgs* default_broadcast_app_args();
static BroadcastAppArgs* parse_broadcast_app_args(const char* file_path);

static void printDiscoveryStats(YggRequest* req);
static void printBcastStats(YggRequest* req);
static void sendMessage(const char* pi, unsigned int counter, BroadcastAppArgs* app_args);
static void rcvMessage(YggMessage* msg, BroadcastAppArgs* app_args);
static void uponNotification(YggEvent* ev, BroadcastAppArgs* app_args);

static void requestDiscoveryStats();
static void requestBroadcastStats();

static void setBroadcastTimer(BroadcastAppArgs* app_args, unsigned char* bcast_timer_id, bool isFirst, struct timespec* start_time);

#define APP_ID 400
#define APP_NAME "BROADCAST APP"

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
    } else {
        broadcast_args = default_broadcast_framework_args();
    }
    registerProtocol(BROADCAST_FRAMEWORK_PROTO_ID, &broadcast_framework_init, (void*) broadcast_args);

    // Broadcast App
    BroadcastAppArgs* app_args = NULL;
    char* app_args_file = hash_table_find_value(args, "app");
    if(app_args_file) {
        app_args = parse_broadcast_app_args(app_args_file);
    } else {
        app_args = default_broadcast_app_args();
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
        setBroadcastTimer(app_args, bcast_timer_id, true, &start_time);
    }

	unsigned int counter = 0;
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
                    sendMessage(hostname, ++counter, app_args);

                    if( counter < app_args->max_broadcasts || app_args->max_broadcasts == ULONG_MAX ) {
						setBroadcastTimer(app_args, bcast_timer_id, false, &start_time);
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
			rcvMessage(&elem.data.msg, app_args);
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

static void sendMessage(const char* hostname, unsigned int counter, BroadcastAppArgs* app_args) {
	char payload[1000];
	struct timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);

    const char* ordinal = ((counter==1) ? "st" : ((counter==2) ? "nd" : ((counter==3) ? "rd" : "th")));

    sprintf(payload, "I'm %s and this is my %d%s message.", hostname, counter, ordinal);

    BroadcastMessage(APP_ID, -1, 0, (byte*)payload, strlen(payload)+1);

    if(app_args->verbose)
	   ygg_log(APP_NAME, "BROADCAST SENT", payload);
}

static void rcvMessage(YggMessage* msg, BroadcastAppArgs* app_args) {
    assert(msg->dataLen > 0 && msg->data != NULL);

    void* ptr = NULL;

    unsigned short src_proto = 0;
    ptr = YggMessage_readPayload(msg, ptr, &src_proto, sizeof(src_proto));
    assert(src_proto == BROADCAST_FRAMEWORK_PROTO_ID);

    unsigned short payload_size = 0;
    ptr = YggMessage_readPayload(msg, ptr, &payload_size, sizeof(payload_size));

    const char* empty_msg = "[EMPTY MESSAGE]";

    unsigned short str_size = payload_size > 0 ? payload_size : strlen(empty_msg)+1;

    char m[str_size];
	//memset(m, 0, str_size);

    if(payload_size > 0) {
        //m[str_size-1] = '\0';
        ptr = YggMessage_readPayload(msg, ptr, m, payload_size);
    } else {
        strcpy(m, empty_msg);
    }

    if(app_args->verbose)
	   ygg_log(APP_NAME, "RECEIVED MESSAGE", m);
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

static void uponNotification(YggEvent* ev, BroadcastAppArgs* app_args) {

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


static void setBroadcastTimer(BroadcastAppArgs* app_args, unsigned char* bcast_timer_id, bool isFirst, struct timespec* start_time) {

    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    subtract_timespec(&current_time, &current_time, start_time);
    unsigned long elapsed_ms = timespec_to_milli(&current_time);

    unsigned long t = getNextDelay(app_args->broadcast_type, elapsed_ms);

    if(isFirst) {
        t += app_args->initial_grace_period_s*1000;
    }

    /*
    #ifdef DEBUG_BROADCAST_TEST
    char str[100];
    sprintf(str, "broadcast_timer = %lu\n", t);
    ygg_log(APP_NAME, "BROADCAST TIMER", str);
    #endif
    */

    struct timespec t_;
    milli_to_timespec(&t_, t);
    SetTimer(&t_, bcast_timer_id, APP_ID, 0);
}

static BroadcastAppArgs* default_broadcast_app_args() {
    BroadcastAppArgs* app_args = malloc(sizeof(BroadcastAppArgs));

    strcpy(app_args->broadcast_type, "Exponential Constant 2.0");
    app_args->initial_grace_period_s = 10;
    app_args->final_grace_period_s = 30;
    app_args->max_broadcasts = ULONG_MAX; // infinite
    app_args->exp_duration_s = 5*60; // 5 min
    app_args->rcv_only = false;
    app_args->verbose = true;

    return app_args;
}

static BroadcastAppArgs* parse_broadcast_app_args(const char* file_path) {
    list* order = list_init();
    hash_table* configs = parse_configs_order(file_path, &order);

    if(configs == NULL) {
        char str[100];
        sprintf(str, "Config file %s not found!", file_path);
        ygg_log(APP_NAME, "ARG ERROR", str);
        ygg_logflush();

        exit(-1);
    }

    BroadcastAppArgs* app_args = default_broadcast_app_args();

    for(list_item* it = order->head; it; it = it->next) {
        char* key = (char*)it->data;
        char* value = (char*)hash_table_find_value(configs, key);

        if( value != NULL ) {
            if( strcmp(key, "broadcast_type") == 0 ) {
                strcpy(app_args->broadcast_type, value);
            } else if( strcmp(key, "initial_grace_period_s") == 0 ) {
                app_args->initial_grace_period_s = (unsigned long) strtol(value, NULL, 10);
            } else if( strcmp(key, "final_grace_period_s") == 0 ) {
                app_args->final_grace_period_s = (unsigned long) strtol(value, NULL, 10);
            } else if( strcmp(key, "max_broadcasts") == 0 ) {
                bool is_infinite = (strcmp(value, "infinite") == 0 || strcmp(value, "infinity") == 0 || strcmp(value, "inf") == 0);
                app_args->max_broadcasts = is_infinite ? ULONG_MAX : (unsigned int) strtol(value, NULL, 10);
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
