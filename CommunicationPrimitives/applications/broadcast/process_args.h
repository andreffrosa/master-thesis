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

 #ifndef _PROCESS_ARGS_H_
 #define _PROCESS_ARGS_H_

#include <getopt.h>
#include <linux/limits.h>

#include "protocols/broadcast/framework/framework.h"
#include "protocols/discovery/topology_discovery.h"

extern struct option long_options[];

#define STR_SIZE 200

typedef struct BroadcastConfigs_ {
    char r_alg[STR_SIZE];
    char r_delay[STR_SIZE];
    char r_policy[STR_SIZE];
    char r_context[STR_SIZE];
    broadcast_framework_args* f_args;
} BroadcastConfigs;

typedef struct DiscoveryConfigs_ {
    unsigned long window_size; // s

    topology_discovery_args* d_args;

    char lq_alg_str[STR_SIZE];
} DiscoveryConfigs;

typedef struct AppConfigs_ {
    char broadcast_type[1000];
    struct timespec start_time;
    unsigned long initial_grace_period_s;
    unsigned long final_grace_period_s;

    //unsigned long initial_timer_ms;
    //unsigned long periodic_timer_ms;
    unsigned long max_broadcasts;
    unsigned long exp_duration_s;
    unsigned char rcv_only;
    //double bcast_prob;
    bool use_overlay;
    char overlay_path[PATH_MAX];
    char interface_name[100];
    char hostname[100];
} AppConfigs;

 typedef struct Configs_ {
     BroadcastConfigs broadcast;
     DiscoveryConfigs discovery;
     AppConfigs app;
 } Configs;

void defaultConfigValues(Configs* config);

void printConfigs(Configs* config);

void processArgs(int argc, char** argv, Configs* config);

void processFile(const char* file_path, Configs* config);

void processLine(char option, int option_index, char* args, Configs* config);

void mountBroadcastAlgorithm(char* token, Configs* config);

void mountRetransmissionDelay(char* args, Configs* config);

void mountRetransmissionPolicy(char* args, Configs* config);

void mountRetransmissionContext(char* args, Configs* config);

void mountLQ(char* args, Configs* config);

 #endif /* _PROCESS_ARGS_H_ */
