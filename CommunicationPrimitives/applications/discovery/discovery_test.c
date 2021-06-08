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

#include <errno.h>

#include "Yggdrasil.h"

#include "../app_common.h"

#include "protocols/discovery/framework/framework.h"

#include "utility/my_math.h"
#include "utility/my_time.h"
#include "utility/my_sys.h"
#include "utility/my_misc.h"
#include "utility/my_string.h"

#include "utility/my_logger.h"

#define APP_ID 400
#define APP_NAME "DISCOVERY APP"


extern void (*log_flush_handler)();

void my_log_flush_handler();

/*static*/ MyLogger* app_logger = NULL;

typedef struct DiscoveryAppArgs_ {
    bool periodic_messages;
    bool verbose;
    unsigned long exp_duration_s;
    //unsigned long max_pings;
    //unsigned long initial_grace_period_s;
    unsigned long final_grace_period_s;
    char timer_type[100];
    // TODO:
} DiscoveryAppArgs;

static DiscoveryAppArgs* default_discovery_app_args();
static DiscoveryAppArgs* load_discovery_app_args(const char* file_path);

static void requestDiscoveryStats();
static void printDiscoveryStats(YggRequest* req);

static void processNotification(YggEvent* notification, DiscoveryAppArgs* app_args);
static void setTimer(unsigned char* timer_id, struct timespec* start_time, struct timespec* current_time, DiscoveryAppArgs* app_args);
static void sendMessage(unsigned char* dest, char* txt);

int main(int argc, char* argv[]) {

    sleep(5);

    srand(time(NULL));

    log_flush_handler = &my_log_flush_handler;

    // Process Args
    hash_table* args = parse_args(argc, argv);

    // Interface and Hostname
    char interface[20] = {0}, hostname[30] = {0};
    unparse_host(hostname, 20, interface, 30, args);

    // Logs
    // char destination[200] = "../experiments/output/discovery/exp1-";
    // hash_table_insert(args, new_str("log"), new_str(strcat(destination, hostname))); // temp

    char* log_path = hash_table_find_value(args, "log");
    if(log_path) {
        // Create dir if does not exist
        int err = r_mkdir(log_path);
        if(err > 0) {
            fprintf(stderr, "Could not create directory %s (errno = %s)\n", log_path, strerror(err));
            exit(-1);
        }

        char file_path[PATH_MAX];
        build_path(file_path, log_path, "app.log");
        //app_logger = new_my_logger(fopen("../experiments/output/discovery/test.txt", "w"), hostname);
        //app_logger = new_my_logger(stdout, hostname);

        /*FILE* f = */freopen(file_path, "w", stdout);
        dup2(1, 2);  //redirects stderr to stdout below this line.

        app_logger = new_my_logger(stdout, hostname);

        build_path(file_path, log_path, "discovery.log");
        discovery_logger = new_my_logger(fopen(file_path, "w"), hostname);

        build_path(file_path, log_path, "neighbors.log");
        neighbors_logger = new_my_logger(fopen(file_path, "w"), hostname);
    } else {
        app_logger = new_my_logger(stdout, hostname);
        discovery_logger = app_logger;
        neighbors_logger = app_logger;
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
    NetworkConfig* ntconf = defineNetworkConfig2(interface, "AdHoc", 2412, 3, 0, "pis", YGG_filter);

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
    discovery_framework_args* discovery_args = NULL;
    char* discovery_args_file = hash_table_find_value(args, "discovery");
    if(discovery_args_file) {
        discovery_args = load_discovery_framework_args(discovery_args_file);
    } else {
        discovery_args = default_discovery_framework_args();
    }
    registerProtocol(DISCOVERY_FRAMEWORK_PROTO_ID, &discovery_framework_init, (void*) discovery_args);

    // Discovery App
    DiscoveryAppArgs* app_args = NULL;
    char* app_args_file = hash_table_find_value(args, "app");
    if(app_args_file) {
        app_args = load_discovery_app_args(app_args_file);
    } else {
        app_args = default_discovery_app_args();
    }
    // TODO: print app_args

    bool finished = false;

    hash_table_delete(args);
    args = NULL;

    // Register this app
    app_def* myApp = create_application_definition(APP_ID, APP_NAME);

    app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEW_NEIGHBOR);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, UPDATE_NEIGHBOR);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, LOST_NEIGHBOR);
    //app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBORHOOD);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, DISCOVERY_ENVIRONMENT_UPDATE);
    //app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, GENERIC_DISCOVERY_EVENT);

    queue_t* inBox = registerApp(myApp);

	// Start ygg_runtime
	ygg_runtime_start();

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Start App

    my_logger_write(app_logger, APP_NAME, "INIT", "init");

    uuid_t myID;
    getmyId(myID);

    struct timespec start_time = {0};
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    struct timespec current_time = start_time;

    struct timespec t = {0};

	// Set end of experiment
	uuid_t end_exp;
	genUUID(end_exp);
	milli_to_timespec(&t, app_args->exp_duration_s*1000);
    SetTimer(&t, end_exp, APP_ID, 0);

    uuid_t kill_exp;
	genUUID(kill_exp);

	// Set timer
    uuid_t send_timer_id;
    genUUID(send_timer_id);
    if(app_args->periodic_messages) {
        setTimer(send_timer_id, &start_time, &current_time, app_args);
    }

    /*
    uuid_t destination_id;
    getmyId(destination_id);
    destination_id[15] = '\003';
    */

	queue_t_elem elem;
	while(1) {
        queue_pop(inBox, &elem);

        // Update current time
		clock_gettime(CLOCK_MONOTONIC, &current_time);

		switch(elem.type) {
		case YGG_TIMER:
            if( uuid_compare(elem.data.timer.id, end_exp ) == 0 ) {
                if(!finished) {
                    finished = true;
                    my_logger_write(app_logger, APP_NAME, "MAIN LOOP", "END.");
                    my_logger_write(discovery_logger, DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "END.");
                    my_logger_write(neighbors_logger, DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "END.");

                    /*my_logger_flush(app_logger);
                    my_logger_flush(discovery_logger);
                    my_logger_flush(neighbors_logger);*/

                    milli_to_timespec(&t, app_args->final_grace_period_s*1000);
                    SetTimer(&t, end_exp, APP_ID, 0);
                } else {
                    requestDiscoveryStats();

                    milli_to_timespec(&t, 500);
                    SetTimer(&t, kill_exp, APP_ID, 0);
                }
            } else if( uuid_compare(elem.data.timer.id, kill_exp) == 0 ) {
                my_log_flush_handler();
                //return 0;
			} else {
                sendMessage(NULL, "msg");
                setTimer(send_timer_id, &start_time, &current_time, app_args);
            }

			break;
		/*case YGG_REQUEST:
			break;*/
		case YGG_MESSAGE:
            if(app_args->verbose) {
                YggMessage* msg = &elem.data.msg;
                my_logger_write(app_logger, APP_NAME, "RECEIVED MESSAGE", msg->data);
            }
			break;
        case YGG_EVENT:
    			if( elem.data.event.proto_dest == APP_ID ) {
    				processNotification(&elem.data.event, app_args);
    			} else {
    				char s[100];
    				sprintf(s, "Received notification from protocol %d meant for protocol %d", elem.data.event.proto_origin, elem.data.event.proto_dest);
    				my_logger_write(app_logger, APP_NAME, "NOTIFICATION", s);
    			}
    			break;
        case YGG_REQUEST:
            ;YggRequest* req = &elem.data.request;
            if( req->proto_dest == APP_ID ) {
                if( req->proto_origin == DISCOVERY_FRAMEWORK_PROTO_ID ) {
                    if( req->request == REPLY && req->request_type == REQ_DISCOVERY_FRAMEWORK_STATS) {
                        printDiscoveryStats(req);
                    }
                }
            }
            break;
		default:
			my_logger_write(app_logger, APP_NAME, "MAIN LOOP", "Received wierd thing in my queue.");
		}

        // Release memory of elem payload
		free_elem_payload(&elem);
	}

	return 0;
}

static void sendMessage(unsigned char* dest_addr, char* txt) {
    YggMessage m;
    if(dest_addr) {
        YggMessage_init(&m, dest_addr, APP_ID);
    } else {
        YggMessage_initBcast(&m, APP_ID);
    }

    YggMessage_addPayload(&m, txt, strlen(txt)+1);
    dispatch(&m);
}

static void setTimer(unsigned char* timer_id, struct timespec* start_time, struct timespec* current_time, DiscoveryAppArgs* app_args) {

    struct timespec elapsed_ms_t;
    subtract_timespec(&elapsed_ms_t, current_time, start_time);
    unsigned long elapsed_ms = timespec_to_milli(&elapsed_ms_t);

    unsigned long t = getNextDelay(app_args->timer_type, elapsed_ms);

    struct timespec t_;
    milli_to_timespec(&t_, t);
    SetTimer(&t_, timer_id, APP_ID, 0);
}

static void processNotification(YggEvent* notification, DiscoveryAppArgs* app_args) {

    if(!app_args->verbose)
        return;

    unsigned short read = 0;
    unsigned char* ptr = notification->payload;

    unsigned short length = 0;
    memcpy(&length, ptr, sizeof(unsigned short));
	ptr += sizeof(unsigned short);
    read += sizeof(unsigned short);

    YggEvent aux = {0};
    YggEvent_init(&aux, DISCOVERY_FRAMEWORK_PROTO_ID, notification->notification_id);
    YggEvent_addPayload(&aux, ptr, length);
    ptr += length;
    read += length;

    {
        char id[UUID_STR_LEN];
        unsigned char* ptr2 = aux.payload;
        if(notification->notification_id == NEW_NEIGHBOR) {
            uuid_unparse(ptr2, id);
    		my_logger_write(app_logger, APP_NAME, "NEW NEIGHBOR", id);

    	} else if(notification->notification_id == UPDATE_NEIGHBOR) {
            uuid_unparse(ptr2, id);
    		my_logger_write(app_logger, APP_NAME, "UPDATE NEIGHBOR", id);

    	} else if(notification->notification_id == LOST_NEIGHBOR) {
            uuid_unparse(ptr2, id);
    		my_logger_write(app_logger, APP_NAME, "LOST NEIGHBOR", id);
    	}
    }

    YggEvent_freePayload(&aux);

    memcpy(&length, ptr, sizeof(unsigned short));
	ptr += sizeof(unsigned short);
    read += sizeof(unsigned short);

    YggEvent neighborhood = {0};
    YggEvent_init(&neighborhood, DISCOVERY_FRAMEWORK_PROTO_ID, 0);
    YggEvent_addPayload(&neighborhood, ptr, length);
    ptr += length;
    read += length;

    {
        //if(notification->notification_id == NEIGHBORHOOD) {
            my_logger_write(app_logger, APP_NAME, "NEIGHBORHOOD", "");
    	//}
    }
    YggEvent_freePayload(&neighborhood);

    while( read < notification->length ) {
        memcpy(&length, ptr, sizeof(unsigned short));
    	ptr += sizeof(unsigned short);
        read += sizeof(unsigned short);

        YggEvent ev = {0};
        YggEvent_init(&ev, DISCOVERY_FRAMEWORK_PROTO_ID, 0);
        YggEvent_addPayload(&ev, ptr, length);
        ptr += length;
        read += length;

        {
            unsigned int str_len = 0;
            void* ptr = NULL;
            ptr = YggEvent_readPayload(&ev, ptr, &str_len, sizeof(unsigned int));

            char type[str_len+1];
            ptr = YggEvent_readPayload(&ev, ptr, type, str_len*sizeof(char));
            type[str_len] = '\0';

            printf("TYPE: %s\n", type);
            if( strcmp(type, "MPRS") == 0 || strcmp(type, "MPR SELECTORS") == 0 ) {

                printf("FLOODING\n");
                unsigned int amount = 0;
                ptr = YggEvent_readPayload(&ev, ptr, &amount, sizeof(unsigned int));

                for(int i = 0; i < amount; i++) {
                    uuid_t id;
                    ptr = YggEvent_readPayload(&ev, ptr, id, sizeof(uuid_t));

                    char id_str[UUID_STR_LEN+1];
                    id_str[UUID_STR_LEN] = '\0';
                    uuid_unparse(id, id_str);
                    printf("%s\n", id_str);
                }

                printf("ROUTING\n");
                amount = 0;
                ptr = YggEvent_readPayload(&ev, ptr, &amount, sizeof(unsigned int));

                for(int i = 0; i < amount; i++) {
                    uuid_t id;
                    ptr = YggEvent_readPayload(&ev, ptr, id, sizeof(uuid_t));

                    char id_str[UUID_STR_LEN+1];
                    id_str[UUID_STR_LEN] = '\0';
                    uuid_unparse(id, id_str);
                    printf("%s\n", id_str);
                }
            }
            else if( strcmp(type, "LENWB_NEIGHS") == 0 ) {
                unsigned int amount = 0;
                ptr = YggEvent_readPayload(&ev, ptr, &amount, sizeof(unsigned int));

                for(int i = 0; i < amount; i++) {
                    uuid_t id;
                    ptr = YggEvent_readPayload(&ev, ptr, id, sizeof(uuid_t));

                    byte n_neighs = 0;
                    ptr = YggEvent_readPayload(&ev, ptr, &n_neighs, sizeof(byte));

                    char id_str[UUID_STR_LEN+1];
                    id_str[UUID_STR_LEN] = '\0';
                    uuid_unparse(id, id_str);
                    printf("%s : %u\n", id_str, n_neighs);
                }
            }
        }

        YggEvent_freePayload(&ev);
    }

}

static DiscoveryAppArgs* default_discovery_app_args() {
    DiscoveryAppArgs* d_args = malloc(sizeof(DiscoveryAppArgs));

    d_args->periodic_messages = false;
    d_args->verbose = false;
    d_args->final_grace_period_s = 30;
    d_args->exp_duration_s = 5*60; // 5 min
    strcpy(d_args->timer_type, "Periodic 1.0 6"); // trans every 6 seconds with 1.0 probability

    return d_args;
}

static DiscoveryAppArgs* load_discovery_app_args(const char* file_path) {

    list* order = list_init();
    hash_table* configs = parse_configs_order(file_path, &order);

    if(configs == NULL) {
        char str[100];
        sprintf(str, "Config file %s not found!", file_path);
        my_logger_write(app_logger, APP_NAME, "ARG ERROR", str);
        ygg_logflush();

        exit(-1);
    }

    DiscoveryAppArgs* d_args = default_discovery_app_args();

    for(list_item* it = order->head; it; it = it->next) {
        char* key = (char*)it->data;
        char* value = (char*)hash_table_find_value(configs, key);

        if( value != NULL ) {
            if( strcmp(key, "periodic_messages") == 0 ) {
                d_args->periodic_messages = strcmp("false", value) == 0 ? false : true;
            } else if( strcmp(key, "verbose") == 0 ) {
                d_args->verbose = strcmp("false", value) == 0 ? false : true;
            } else if( strcmp(key, "timer_type") == 0 ) {
                strcpy(d_args->timer_type, value);
            } else if( strcmp(key, "exp_duration_s") == 0 ) {
                d_args->exp_duration_s = strtol(value, NULL, 10);
            } else if( strcmp(key, "final_grace_period_s") == 0 ) {
                d_args->final_grace_period_s = strtol(value, NULL, 10);
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

    return d_args;
}

static void requestDiscoveryStats() {
    YggRequest req;
	YggRequest_init(&req, APP_ID, DISCOVERY_FRAMEWORK_PROTO_ID, REQUEST, REQ_DISCOVERY_FRAMEWORK_STATS);
    deliverRequest(&req);
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

void my_log_flush_handler() {

    my_logger_write(app_logger, APP_NAME, "MAIN LOOP", "KILL");
    my_logger_write(discovery_logger, DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "KILL");
    my_logger_write(neighbors_logger, DISCOVERY_FRAMEWORK_PROTO_NAME, "MAIN LOOP", "KILL");

    my_logger_flush(app_logger);
    my_logger_flush(discovery_logger);
    my_logger_flush(neighbors_logger);

}
