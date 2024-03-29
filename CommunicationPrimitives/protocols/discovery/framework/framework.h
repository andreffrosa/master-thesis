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

#ifndef _DISCOVERY_FRAMEWORK_H_
#define _DISCOVERY_FRAMEWORK_H_

#include "Yggdrasil.h"

#include "data_structures/double_list.h"

#include "discovery_algorithm/discovery_algorithms.h"

#define DISCOVERY_FRAMEWORK_PROTO_ID 159
#define DISCOVERY_FRAMEWORK_PROTO_NAME "DISCOVERY FRAMEWORK"

typedef struct discovery_stats_ {
    unsigned long new_neighbors;
	unsigned long lost_neighbors;
    unsigned long updated_neighbors;

    unsigned long discovery_messages;
    unsigned long total_hellos;
    unsigned long total_hacks;
    unsigned long piggybacked_hellos;
    unsigned long piggybacked_hacks;

    unsigned long missed_hellos;
    unsigned long missed_hacks;
} discovery_stats;

typedef struct discovery_framework_args_ {
    DiscoveryAlgorithm* algorithm;

    unsigned int hello_misses;
    unsigned int hack_misses;

    unsigned long neigh_hold_time_s;
    unsigned long max_jitter_ms;
    unsigned long period_margin_ms;
    unsigned int announce_transition_period_n;

    unsigned long min_hello_interval_ms;
    unsigned long min_hack_interval_ms;

    bool ignore_zero_seq;

    double lq_epsilon;
    double lq_threshold;
    int lq_precision;

    double traffic_threshold;
    double traffic_epsilon;
    int traffic_precision;

    unsigned int discov_env_refresh_period_s;
    unsigned int traffic_n_bucket;
    unsigned int traffic_bucket_duration_s;
    unsigned int churn_n_bucket;
    unsigned int churn_bucket_duration_s;
    char traffic_window_type[10];
    char churn_window_type[10];
    double churn_epsilon;
    int churn_precision;
    double neigh_density_epsilon;
    int neigh_density_precision;

    bool toggle_env;
} discovery_framework_args;

proto_def* discovery_framework_init(void* arg);

void* discovery_framework_main_loop(main_loop_args* args);

discovery_framework_args* new_discovery_framework_args(DiscoveryAlgorithm* algorithm, unsigned int hello_misses, unsigned int hack_misses, unsigned long neigh_hold_time_s, unsigned long max_jitter_ms, unsigned long period_margin_ms, unsigned int announce_transition_period_n, unsigned long min_hello_interval_ms, unsigned long min_hack_interval_ms, bool ignore_zero_seq, double lq_epsilon, int lq_precision, double lq_threshold, double traffic_epsilon, int traffic_precision, double traffic_threshold, unsigned int discov_env_refresh_period_s, unsigned int traffic_n_bucket, unsigned int traffic_bucket_duration_s, unsigned int churn_n_bucket, unsigned int churn_bucket_duration_s, char* traffic_window_type, char* churn_window_type, double churn_epsilon, int churn_precision, double neigh_density_epsilon, int neigh_density_precision, bool toggle_env);

discovery_framework_args* default_discovery_framework_args();

discovery_framework_args* load_discovery_framework_args(const char* file_path);

typedef enum {
	NEW_NEIGHBOR,
	UPDATE_NEIGHBOR,
	LOST_NEIGHBOR,
    //NEIGHBORHOOD,
    //GENERIC_DISCOVERY_EVENT,
    DISCOVERY_ENVIRONMENT_UPDATE,
	DISCOVERY_EVENT_COUNT
} DiscoveryEventType;

typedef enum {
	REQ_DISCOVERY_FRAMEWORK_STATS = 0,
    DISCOVERY_REQ_TYPE_COUNT
} DiscoveryRequestType;

typedef enum {
	HELLO_TIMER = 0,
    HACK_TIMER,
    REPLY_TIMER,
    NEIGHBOR_CHANGE_TIMER,
    DISCOVERY_ENVIRONMENT_TIMER,
    NEIGHBOR_TIMER,
	DISCOVERY_TIMER_TYPE_COUNT
} DiscoveryTimerType;

#endif /* _DISCOVERY_FRAMEWORK_H_ */
