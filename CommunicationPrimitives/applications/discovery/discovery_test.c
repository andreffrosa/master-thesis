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

#include "protocols/discovery/framework/framework.h"

#include "utility/my_math.h"
#include "utility/my_time.h"
#include "utility/my_sys.h"
#include "utility/my_misc.h"

#define APP_ID 400
#define APP_NAME "DISCOVERY APP"

typedef struct DiscoveryAppArgs_ {
    bool periodic_messages;
    bool verbose;
    // TODO:
} DiscoveryAppArgs;

static DiscoveryAppArgs* default_discovery_app_args();
static DiscoveryAppArgs* load_discovery_app_args(const char* file_path);

static void processNotification(YggEvent* notification, DiscoveryAppArgs* app_args);

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

    hash_table_delete(args);
    args = NULL;

    // Register this app
    app_def* myApp = create_application_definition(APP_ID, APP_NAME);

    app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEW_NEIGHBOR);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, UPDATE_NEIGHBOR);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, LOST_NEIGHBOR);
    app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBORHOOD);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, DISCOVERY_ENVIRONMENT_UPDATE);
    app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, GENERIC_DISCOVERY_EVENT);

    queue_t* inBox = registerApp(myApp);

	// Start ygg_runtime
	ygg_runtime_start();

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Start App

	// Set periodic timer
    struct timespec t;
    milli_to_timespec(&t, 6000);
    uuid_t tid;
    genUUID(tid);
    if(app_args->periodic_messages) {
        SetPeriodicTimer(&t, tid, APP_ID, -1);
    }

    ygg_log(APP_NAME, "INIT", "init");


    uuid_t myID, destination_id;
    getmyId(myID);
    getmyId(destination_id);
    destination_id[15] = '\003';

	queue_t_elem elem;
	while(1) {
		queue_pop(inBox, &elem);

		switch(elem.type) {
		case YGG_TIMER:
            ; YggMessage m;

            YggMessage_initBcast(&m, APP_ID);
            char* str = "msg";
            YggMessage_addPayload(&m, str, strlen(str)+1);
            dispatch(&m);
			break;
		/*case YGG_REQUEST:
			break;*/
		case YGG_MESSAGE:
            if(app_args->verbose) {
                YggMessage* msg = &elem.data.msg;
                ygg_log(APP_NAME, "RCV MESSAGE", msg->data);
            }
			break;
        case YGG_EVENT:
    			if( elem.data.event.proto_dest == APP_ID ) {
    				processNotification(&elem.data.event, app_args);
    			} else {
    				char s[100];
    				sprintf(s, "Received notification from protocol %d meant for protocol %d", elem.data.event.proto_origin, elem.data.event.proto_dest);
    				ygg_log("BCAST TEST APP", "NOTIFICATION", s);
    			}
    			break;
		default:
			ygg_log(APP_NAME, "MAIN LOOP", "Received wierd thing in my queue.");
		}

        // Release memory of elem payload
		free_elem_payload(&elem);
	}

	return 0;
}

static void processNotification(YggEvent* notification, DiscoveryAppArgs* app_args) {

    if(!app_args->verbose)
        return;

    char id[UUID_STR_LEN+1];
    id[UUID_STR_LEN] = '\0';
	unsigned char* ptr = notification->payload;

	if(notification->notification_id == NEW_NEIGHBOR) {
        uuid_unparse(ptr, id);
		ygg_log(APP_NAME, "NEW NEIGHBOR", id);

	} else if(notification->notification_id == UPDATE_NEIGHBOR) {
        uuid_unparse(ptr, id);
		ygg_log(APP_NAME, "UPDATE NEIGHBOR", id);

	} else if(notification->notification_id == LOST_NEIGHBOR) {
        uuid_unparse(ptr, id);
		ygg_log(APP_NAME, "LOST NEIGHBOR", id);
	} else if(notification->notification_id == NEIGHBORHOOD) {
        ygg_log(APP_NAME, "NEIGHBORHOOD", "");
	} else if(notification->notification_id == DISCOVERY_ENVIRONMENT_UPDATE) {
        ygg_log(APP_NAME, "DISCOVERY ENVIRONMENT UPDATE", "");
	}
    else if(notification->notification_id == GENERIC_DISCOVERY_EVENT) {

        unsigned int str_len = 0;
        void* ptr = NULL;
        ptr = YggEvent_readPayload(notification, ptr, &str_len, sizeof(unsigned int));

        char type[str_len+1];
        ptr = YggEvent_readPayload(notification, ptr, type, str_len*sizeof(char));
        type[str_len] = '\0';

        printf("TYPE: %s\n", type);

        if( strcmp(type, "MPRS") == 0 || strcmp(type, "MPR SELECTORS") == 0 ) {

                printf("FLOODING\n");
                unsigned int amount = 0;
                ptr = YggEvent_readPayload(notification, ptr, &amount, sizeof(unsigned int));

                for(int i = 0; i < amount; i++) {
                    uuid_t id;
                    ptr = YggEvent_readPayload(notification, ptr, id, sizeof(uuid_t));

                    char id_str[UUID_STR_LEN+1];
                    id_str[UUID_STR_LEN] = '\0';
                    uuid_unparse(id, id_str);
                    printf("%s\n", id_str);
                }

                printf("ROUTING\n");
                amount = 0;
                ptr = YggEvent_readPayload(notification, ptr, &amount, sizeof(unsigned int));

                for(int i = 0; i < amount; i++) {
                    uuid_t id;
                    ptr = YggEvent_readPayload(notification, ptr, id, sizeof(uuid_t));

                    char id_str[UUID_STR_LEN+1];
                    id_str[UUID_STR_LEN] = '\0';
                    uuid_unparse(id, id_str);
                    printf("%s\n", id_str);
                }
        }
        else if( strcmp(type, "LENWB_NEIGHS") == 0 ) {
            unsigned int amount = 0;
            ptr = YggEvent_readPayload(notification, ptr, &amount, sizeof(unsigned int));

            for(int i = 0; i < amount; i++) {
                uuid_t id;
                ptr = YggEvent_readPayload(notification, ptr, id, sizeof(uuid_t));

                byte n_neighs = 0;
                ptr = YggEvent_readPayload(notification, ptr, &n_neighs, sizeof(byte));

                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(id, id_str);
                printf("%s : %u\n", id_str, n_neighs);
            }
        }

	}
}

static DiscoveryAppArgs* default_discovery_app_args() {
    DiscoveryAppArgs* d_args = malloc(sizeof(DiscoveryAppArgs));

    d_args->periodic_messages = false;
    d_args->verbose = false;

    return d_args;
}

static DiscoveryAppArgs* load_discovery_app_args(const char* file_path) {

    list* order = list_init();
    hash_table* configs = parse_configs_order(file_path, &order);

    if(configs == NULL) {
        char str[100];
        sprintf(str, "Config file %s not found!", file_path);
        ygg_log(APP_NAME, "ARG ERROR", str);
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

    return d_args;
}
