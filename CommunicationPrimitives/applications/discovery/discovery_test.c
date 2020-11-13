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

    bool periodic_messages = strcmp("false", argv[2]) == 0 ? false : true;

	NetworkConfig* ntconf = defineNetworkConfig2(interface, "AdHoc", 2462, 3, 1, "pis", YGG_filter);

	// Initialize ygg_runtime
	ygg_runtime_init_2(ntconf, hostname);

    //const char* pi = getHostname(); // raspi-n
    //pi = (pi == NULL) ? config.app.hostname : pi;

	// Register this app
	app_def* myApp = create_application_definition(APP_ID, APP_NAME);

    // Register Framework Protocol
    discovery_framework_args* f_args = malloc(sizeof(discovery_framework_args));
    //f_args.algorithm = ActiveDiscovery(3000, TopologyAnnounceModule(3), OLSRLinkQuality(0.3, true)); //HybridDiscovery(3000, OLSRLinkQuality(0.3, true)); //ActiveDiscovery(3000, OLSRAnnounceModule(), OLSRLinkQuality(0.3, true)); //PassiveDiscovery(OLSRLinkQuality(0.3, true));
    /*
f_args.neigh_validity_s = 15;
    f_args.neigh_hold_time_s = 6;
    f_args.max_announce_jitter_ms = 150;
    f_args.process_hb_on_active = false;
    f_args.lq_threshold = 0.1;
    f_args.window_duration_s = 10;
    f_args.window_notify_period_s = 5;
    f_args.window_type = "wma";
    f_args.flush_events_upon_announce = false;
*/

 // TODO: isto depende de alg para alg, devia ser part do alg e não da framework

    f_args->algorithm = newDiscoveryAlgorithm(
        PeriodicJointDiscovery(true, true, true, true, true, true),
        //EchoDiscovery(BROADCAST_HACK_REPLY, true, false),   // Discovery Pattern
        StaticDiscoveryPeriod(5, 5),                        // Discovery Period
        EMALinkQuality(0.5, 0.7, 5, 5),                    // LinkQuality
        OLSRDiscoveryMessage()                              // Discovery Message
    );
    f_args->neigh_hold_time_s = 15;
    f_args->max_jitter_ms = 500;
    f_args->period_margin_ms = 700;

    f_args->hello_misses = 3;
    f_args->hack_misses = 2;

    f_args->ignore_zero_seq = true;

    f_args->lq_epsilon = 0.01;
    f_args->lq_threshold = 0.3;
    f_args->traffic_epsilon = 0.1;
    f_args->traffic_threshold = 1.0;

    f_args->n_buckets = 5;
    f_args->bucket_duration_s = 10;
    f_args->window_notify_period_s = 0;
    f_args->window_type = "ema 0.65";

	registerProtocol(DISCOVERY_FRAMEWORK_PROTO_ID, &discovery_framework_init, (void*) f_args);
    app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_FOUND);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_UPDATE);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, NEIGHBOR_LOST);
	app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, WINDOWS_EVENT);
    app_def_add_consumed_events(myApp, DISCOVERY_FRAMEWORK_PROTO_ID, GENERIC_DISCOVERY_EVENT);

    bool use_overlay = argc == 4;
    if( use_overlay ) {
        char* overlay_path = argv[3];
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

	/*char str[100];
	sprintf(str, "%s starting experience with duration %lu + %lu + %lu s\n", pi, config.app.initial_grace_period_s, config.app.exp_duration_s, config.app.final_grace_period_s);
	ygg_log(APP_NAME, "INIT", str);*/
    ygg_log(APP_NAME, "INIT", "init");


    uuid_t myID, destination_id;
    getmyId(myID);
    getmyId(destination_id);
    destination_id[15] = '\003';

    /*
    char msg[] = "Ya bina, nao desatina!";

    if(ix == 1) {
        RouteMessage(destination_id, APP_ID, (unsigned char*)msg, strlen(msg)+1);
    }
    */

	queue_t_elem elem;
	while(1) {
		queue_pop(inBox, &elem);

		switch(elem.type) {
		case YGG_TIMER:
            ; YggMessage m;
            // getMyWLANAddr()
            //WLANAddr addr;
            //str2wlan((char*)addr.data, "b8:27:eb:9a:68:47"); // raspi-2
            //YggMessage_init(&m, addr.data, APP_ID);

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

    if(notification->notification_id != WINDOWS_EVENT && notification->notification_id != WINDOWS_EVENT) {
		uuid_unparse(ptr, id);
	}

	if(notification->notification_id == NEIGHBOR_FOUND) {
		ygg_log(APP_NAME, "NEIGHBOR FOUND", id);

        /*
    ptr = ((unsigned char*)notification->payload) + sizeof(uuid_t) + WLAN_ADDR_LEN + sizeof(DiscoveryNeighborType);
        unsigned char n_neighs = 0;
        memcpy(&n_neighs, ptr, sizeof(n_neighs));
        printf("neighbors %d\n", n_neighs);
*/



	} else if(notification->notification_id == NEIGHBOR_UPDATE) {
		ygg_log(APP_NAME, "NEIGHBOR UPDATE", id);

        /*
ptr = ((unsigned char*)notification->payload) + sizeof(uuid_t) + WLAN_ADDR_LEN + sizeof(DiscoveryNeighborType);
        unsigned char n_neighs = 0;
        memcpy(&n_neighs, ptr, sizeof(n_neighs));
        printf("neighbors %d\n", n_neighs);
*/



	} else if(notification->notification_id == NEIGHBOR_LOST) {
		ygg_log(APP_NAME, "NEIGHBOR LOST", id);
	} else if(notification->notification_id == WINDOWS_EVENT) {
		/*ygg_log("DISCOVERY TEST APP", "NEIGHBORHOOD UPDATE", id);

        char* str;
    	printAnnounce(notification->payload, notification->length, -1, &str);
    	printf("%s\n%s", "Serialized Announce", str);
    	free(str);*/
        ygg_log(APP_NAME, "WINDOWS", "");
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

                for(int i = amount; i < amount; i++) {
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

                for(int i = amount; i < amount; i++) {
                    uuid_t id;
                    ptr = YggEvent_readPayload(notification, ptr, id, sizeof(uuid_t));

                    char id_str[UUID_STR_LEN+1];
                    id_str[UUID_STR_LEN] = '\0';
                    uuid_unparse(id, id_str);
                    printf("%s\n", id_str);
                }
        }

		/*ygg_log("DISCOVERY TEST APP", "NEIGHBORHOOD UPDATE", id);

        char* str;
    	printAnnounce(notification->payload, notification->length, -1, &str);
    	printf("%s\n%s", "Serialized Announce", str);
    	free(str);*/
        ygg_log(APP_NAME, "WINDOWS", "");
	}
}
