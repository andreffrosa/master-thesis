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

#include "protocols/discovery/framework/framework.h"

#include "utility/my_math.h"
#include "utility/my_time.h"
#include "utility/my_sys.h"
#include "utility/my_misc.h"

#define APP_ID 400
#define APP_NAME "DISCOVERY FRAMEWORK APP"

static void processNotification(YggEvent* notification);

int main(int argc, char* argv[]) {

    assert(argc >= 3);

    int ix = atoi(argv[1]);
    char interface[10];
    sprintf(interface, "wlan%d", ix-1);
    char hostname[30];
    sprintf(hostname, "raspi-0%d", ix);

    bool periodic_messages = strcmp("false", argv[3]) == 0 ? false : true;

	NetworkConfig* ntconf = defineNetworkConfig2(interface, "AdHoc", 2462, 3, 1, "pis", YGG_filter);

	// Initialize ygg_runtime
	ygg_runtime_init_2(ntconf, hostname);

	// Register this app
	app_def* myApp = create_application_definition(APP_ID, APP_NAME);

    // Register Framework Protocol
    const char* discovery_configs = argv[2];
    discovery_framework_args* f_args = load_discovery_framework_args(discovery_configs);

	registerProtocol(DISCOVERY_FRAMEWORK_PROTO_ID, &discovery_framework_init, (void*) f_args);
    app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_FOUND);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_UPDATE);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_LOST);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, DISCOVERY_ENVIRONMENT_UPDATE);
    app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, GENERIC_DISCOVERY_EVENT);

    bool use_overlay = argc == 5;
    if( use_overlay ) {
        char* overlay_path = argv[4];
        topology_manager_args* t_args = load_overlay(overlay_path, hostname);
        registerYggProtocol(PROTO_TOPOLOGY_MANAGER, topologyManager_init, t_args);
		topology_manager_args_destroy(t_args);
    }

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
    if(periodic_messages) {
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
            ;
            YggMessage* msg = &elem.data.msg;
            ygg_log(APP_NAME, "RCV MESSAGE", msg->data);
			break;
        case YGG_EVENT:
    			if( elem.data.event.proto_dest == APP_ID ) {
    				processNotification(&elem.data.event);
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

static void processNotification(YggEvent* notification) {

    char id[UUID_STR_LEN+1];
    id[UUID_STR_LEN] = '\0';
	unsigned char* ptr = notification->payload;

	if(notification->notification_id == NEIGHBOR_FOUND) {
        uuid_unparse(ptr, id);
		ygg_log(APP_NAME, "NEIGHBOR FOUND", id);

	} else if(notification->notification_id == NEIGHBOR_UPDATE) {
        uuid_unparse(ptr, id);
		ygg_log(APP_NAME, "NEIGHBOR UPDATE", id);

	} else if(notification->notification_id == NEIGHBOR_LOST) {
        uuid_unparse(ptr, id);
		ygg_log(APP_NAME, "NEIGHBOR LOST", id);
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

	}
}
