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

#include "protocols/broadcast/framework/framework.h"
#include "protocols/discovery/topology_discovery.h"
#include "utility/my_math.h"
#include "utility/my_time.h"
#include "utility/my_sys.h"

#include "process_args.h"

static void printDiscoveryStats(YggRequest* req);
static void printBcastStats(YggRequest* req);
static void bcastMessage(const char* pi, unsigned int counter);
static void rcvMessage(YggMessage* msg);
static void uponNotification(YggEvent* ev, Configs* config);

static unsigned long getTimerDuration(Configs* config, bool isFirst);
static void setBroadcastTimer(Configs* config, unsigned char* bcast_timer_id, bool isFirst);

#define APP_ID 400
#define APP_NAME "BROADCAST APP"

int main(int argc, char* argv[]) {

	// Process Args
    Configs config;
    defaultConfigValues(&config);
	processArgs(argc, argv, &config);
    printConfigs(&config);

	NetworkConfig* ntconf = defineNetworkConfig2(config.app.interface_name, "AdHoc", 2462, 3, 1, "pis", YGG_filter);

	// Initialize ygg_runtime
	ygg_runtime_init_2(ntconf, config.app.hostname);

    const char* pi = getHostname(); // raspi-n
    pi = (pi == NULL) ? config.app.hostname : pi;

	// Register this app
	app_def* myApp = create_application_definition(APP_ID, APP_NAME);

	if(config.app.use_overlay) {
		// Register Topology Manager Protocol
		char db_file_path[PATH_MAX];
		char neighs_file_path[PATH_MAX];
		build_path(db_file_path, config.app.overlay_path, "macAddrDB.txt");
		//build_path(neighs_file_path, config.app.overlay_path, "neighs.txt");
        char neighs_file[20];
        sprintf(neighs_file, "%s.txt", pi);
        build_path(neighs_file_path, config.app.overlay_path, neighs_file);

        char str[1000];
        char cmd[PATH_MAX + 300];
        sprintf(cmd, "cat %s | wc -l", db_file_path);
        int n = run_command(cmd, str, 1000);
        assert(n > 0);

        int db_size = (int) strtol(str, NULL, 10);

		topology_manager_args* t_args = topology_manager_args_init(db_size, db_file_path, neighs_file_path, true);
		registerYggProtocol(PROTO_TOPOLOGY_MANAGER, topologyManager_init, t_args);
		topology_manager_args_destroy(t_args);
	}

	// Register Framework Protocol
	registerProtocol(BCAST_FRAMEWORK_PROTO_ID, &broadcast_framework_init, (void*) config.broadcast.f_args);

    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBOR_FOUND);
	app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBOR_UPDATE);
	app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBOR_LOST);
    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, NEIGHBORHOOD_UPDATE);
    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, IN_TRAFFIC);
    app_def_add_consumed_events(myApp, TOPOLOGY_DISCOVERY_PROTO_ID, OUT_TRAFFIC);

    queue_t* inBox = registerApp(myApp);

	// Start ygg_runtime
	ygg_runtime_start();

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Start App

	// Set periodic timer
	struct timespec t = {0};

    clock_gettime(CLOCK_MONOTONIC, &config.app.start_time);

    uuid_t bcast_timer_id;
	genUUID(bcast_timer_id);
    if(!config.app.rcv_only){
        setBroadcastTimer(&config, bcast_timer_id, true);
    }

	// Set end of experiment
	uuid_t end_exp;
	genUUID(end_exp);
	milli_to_timespec(&t, config.app.exp_duration_s*1000 + config.app.initial_grace_period_s*1000);
    SetTimer(&t, end_exp, APP_ID, 0);

	YggRequest discovery_stats_req;
	YggRequest_init(&discovery_stats_req, APP_ID, TOPOLOGY_DISCOVERY_PROTO_ID, REQUEST, STATS_REQ);
	YggRequest framework_stats_req;
	YggRequest_init(&framework_stats_req, APP_ID, BCAST_FRAMEWORK_PROTO_ID, REQUEST, REQ_BCAST_FRAMEWORK_STATS);

	char str[100];
	sprintf(str, "%s starting experience with duration %lu + %lu + %lu s\n", pi, config.app.initial_grace_period_s, config.app.exp_duration_s, config.app.final_grace_period_s);
	ygg_log(APP_NAME, "INIT", str);

	unsigned int counter = 0;
    bool finished = false;

	queue_t_elem elem;
	while(1) {
		queue_pop(inBox, &elem);

		switch(elem.type) {
		case YGG_TIMER:
			// Send new msg with probability send_prob
			if( uuid_compare(elem.data.timer.id, end_exp ) == 0 ) {
                if(!finished) {
                    finished = true;
                    ygg_log(APP_NAME, "MAIN LOOP", "END.");

                    milli_to_timespec(&t, config.app.final_grace_period_s*1000);
                    SetTimer(&t, end_exp, APP_ID, 0);
                } else {
                    deliverRequest(&framework_stats_req);
                    deliverRequest(&discovery_stats_req);
                }
			} else if( uuid_compare(elem.data.timer.id, bcast_timer_id ) == 0 ) {

				if (!config.app.rcv_only && !finished) {
                    bcastMessage(pi, ++counter);

                    if( counter < config.app.max_broadcasts || config.app.max_broadcasts == ULONG_MAX ) {
						setBroadcastTimer(&config, bcast_timer_id, false);
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

				if( req->proto_origin == TOPOLOGY_DISCOVERY_PROTO_ID ) {
					if( req->request == REPLY && req->request_type == STATS_REQ) {
						printDiscoveryStats(req);
					}
				} else if ( req->proto_origin == BCAST_FRAMEWORK_PROTO_ID ) {
					if( req->request == REPLY && req->request_type == REQ_BCAST_FRAMEWORK_STATS) {
						printBcastStats(req);
					}
				}
			}
			break;
		case YGG_MESSAGE:
			rcvMessage(&elem.data.msg);
            /*if(finished) {
                deliverRequest(&framework_stats_req);
                deliverRequest(&discovery_stats_req);
            }*/
			break;
		case YGG_EVENT:
			uponNotification(&elem.data.event, &config);
			break;
		default:
			ygg_log(APP_NAME, "MAIN LOOP", "Received wierd thing in my queue.");
		}

        // Release memory of elem payload
		free_elem_payload(&elem);
	}

	return 0;
}

static void bcastMessage(const char* pi, unsigned int counter) {
	char payload[1000];
	struct timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);

    const char* ordinal = ((counter==1) ? "st" : ((counter==2) ? "nd" : ((counter==3) ? "rd" : "th")));
    sprintf(payload, "I'm %s and this is my %d%s message.", pi, counter, ordinal);

    BroadcastMessage(APP_ID, (unsigned char*)payload, strlen(payload)+1, -1);

	ygg_log(APP_NAME, "BROADCAST SENT", payload);
}

static void rcvMessage(YggMessage* msg) {
	char m[1000];
	memset(m, 0, 1000);

	struct timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);

	if (msg->dataLen > 0 && msg->data != NULL) {
		memcpy(m, msg->data, msg->dataLen);
	} else {
		strcpy(m, "[EMPTY MESSAGE]");
	}
	//sprintf(m, "%s Received at %ld:%ld.", m, current_time.tv_sec, current_time.tv_nsec);
	ygg_log(APP_NAME, "RECEIVED MESSAGE", m);
}

static void printBcastStats(YggRequest* req) {
	broadcast_stats stats;
	YggRequest_readPayload(req, NULL, &stats, sizeof(broadcast_stats));

	char m[1000];
	sprintf(m, "Broadcast: %d \t Received: %d \t Transmitted: %d \t Not_Transmitted: %d \t Delivered: %d",
            stats.messages_bcasted,
            stats.messages_received,
            stats.messages_transmitted,
			stats.messages_not_transmitted,
            stats.messages_delivered);

	ygg_log(APP_NAME, "BROADCAST STATS", m);
}

static void printDiscoveryStats(YggRequest* req) {
	topology_discovery_stats stats;
	YggRequest_readPayload(req, NULL, &stats, sizeof(topology_discovery_stats));

	char str[1000];
	sprintf(str, "Announces: %d \t Piggybacked: %d \t Lost Neighs: %d \t New Neighs: %d",
            stats.announces,
            stats.piggybacked_heartbeats,
            stats.lost_neighbours,
            stats.new_neighbours);

	ygg_log(APP_NAME, "DISCOVERY STATS", str);
}

static void uponNotification(YggEvent* ev, Configs* config) {

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

}

static unsigned long getTimerDuration(Configs* config, bool isFirst) {
    unsigned long t = 0L;

    char aux[strlen(config->app.broadcast_type)+1];
    strcpy(aux, config->app.broadcast_type);

    char* ptr = NULL;
    char* token  = strtok_r(aux, " ", &ptr);

    if(token == NULL) {
        fprintf(stderr, "No parameter passed");
        exit(-1);
    }
    char* name;
    if(strcmp(token, (name = "Exponential")) == 0) {
        double lambda = 0.0;

        token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
            if(strcmp(token, "Oscillating") == 0) {
                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
                    double dev = strtod(token, NULL);

                    token = strtok_r(NULL, " ", &ptr);
            		if(token != NULL) {
                        double stretching = strtod(token, NULL);

                        token = strtok_r(NULL, " ", &ptr);
                		if(token != NULL) {
                            double min_lambda = strtod(token, NULL);

                            token = strtok_r(NULL, " ", &ptr);
                    		if(token != NULL) {
                                double max_lambda = strtod(token, NULL);

                                struct timespec current_time;
                                clock_gettime(CLOCK_MONOTONIC, &current_time);
                                subtract_timespec(&current_time, &current_time, &config->app.start_time);
                                double seconds = timespec_to_milli(&current_time) / 1000.0;

                                double aux = dev + stretching*(seconds / config->app.exp_duration_s);
                                double f = (1.0+cos(2.0*M_PI*aux))/2.0;

                                lambda = min_lambda + (max_lambda - min_lambda)*f;
                                //printf("lambda = %f seconds = %f\n", lambda, seconds);
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
            } else if(strcmp(token, "Constant") == 0) {
                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
                    lambda = strtod(token, NULL);
        		} else {
        			printf("Parameter 2 of %s not passed!\n", name);
        			exit(-1);
        		}
            }
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}

        t = round(randomExponential(lambda/1000.0)); // t is in millis, lambda is transmissions per second
    } else if(strcmp(token, (name="Periodic")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double bcast_prob = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                unsigned long periodic_timer_ms = strtol(token, NULL, 10);

                int counter = 1;
                while( bcast_prob < 1.0 && getRandomProb() > bcast_prob )
                    counter++;
                t = counter*periodic_timer_ms;
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    }

    if(isFirst) {
        t += config->app.initial_grace_period_s*1000;
    }

    #ifdef DEBUG_BROADCAST_TEST
    char str[100];
    sprintf(str, "broadcast_timer = %lu\n", t);
    ygg_log(APP_NAME, "BROADCAST TIMER", str);
    #endif

    return t;
}

static void setBroadcastTimer(Configs* config, unsigned char* bcast_timer_id, bool isFirst) {
    unsigned long t = getTimerDuration(config, isFirst);
    struct timespec t_;
    milli_to_timespec(&t_, t);
    SetTimer(&t_, bcast_timer_id, APP_ID, 0);
}
