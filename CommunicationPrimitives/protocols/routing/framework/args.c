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

#include "framework.h"

#include "utility/my_sys.h"
#include "utility/my_string.h"

#include <assert.h>

routing_framework_args* new_routing_framework_args(RoutingAlgorithm* algorithm, unsigned long seen_expiration_ms, unsigned long gc_interval_s) {

    routing_framework_args* args = malloc(sizeof(routing_framework_args));

    args->algorithm = algorithm;
    args->seen_expiration_ms = seen_expiration_ms;
    args->gc_interval_s = gc_interval_s;

    // args->ignore_zero_seq = ignore_zero_seq;

    return args;
}

routing_framework_args* default_routing_framework_args() {

    RoutingAlgorithm* algorithm = newRoutingAlgorithm(
        StaticRoutingContext(),
        ConventionalRouting()
    );

    return new_routing_framework_args(
        algorithm, //RoutingAlgorithm* algorithm,
        60*1000,
        60*1000
    );
}

//static RoutingPeriod* parse_r_period(char* value, bool nested);
//static RoutingContext* parse_r_context(char* value, bool nested);
//static ForwardingStrategy* parse_f_strategy(char* value, bool nested);

routing_framework_args* load_routing_framework_args(const char* file_path) {

    return default_routing_framework_args();


    /*list* order = list_init();
    hash_table* configs = parse_configs_order(file_path, &order);

    if(configs == NULL) {
        char str[100];
        sprintf(str, "Config file %s not found!", file_path);
        ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
        ygg_logflush();

        exit(-1);
    }

    routing_framework_args* args = default_routing_framework_args();

    for(list_item* it = order->head; it; it = it->next) {
        char* key = (char*)it->data;
        char* value = (char*)hash_table_find_value(configs, key);

        if( value != NULL ) {
            if( strcmp(key, "algorithm") == 0 ) {
                // TODO
            } else if( strcmp(key, "d_context") == 0 ) {
                RoutingContext* new_d_context = parse_d_context(value, false);
                DA_setRoutingContext(args->algorithm, new_d_context);
            } else {
                char str[50];
                sprintf(str, "Unknown Config %s = %s", key, value);
                ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
            }
        } else {
            char str[50];
            sprintf(str, "Empty Config %s", key);
            ygg_log(DISCOVERY_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
        }

    }

    // Clean
    hash_table_delete(configs);
    list_delete(order);

    return args;*/
}
