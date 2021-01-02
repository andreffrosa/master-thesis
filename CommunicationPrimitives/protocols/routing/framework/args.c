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

routing_framework_args* new_routing_framework_args(RoutingAlgorithm* algorithm, unsigned long seen_expiration_ms, unsigned long gc_interval_s, unsigned long max_jitter_ms, unsigned long period_margin_ms, bool ignore_zero_seq) {

    routing_framework_args* args = malloc(sizeof(routing_framework_args));

    args->algorithm = algorithm;
    args->seen_expiration_ms = seen_expiration_ms;
    args->gc_interval_s = gc_interval_s;

    args->ignore_zero_seq = ignore_zero_seq;

    args->max_jitter_ms = max_jitter_ms;
    args->period_margin_ms = period_margin_ms;

    return args;
}

routing_framework_args* default_routing_framework_args() {

    RoutingAlgorithm* algorithm = newRoutingAlgorithm(
        StaticRoutingContext(),
        ConventionalRouting(),
        HopsMetric(),
        StaticAnnouncePeriod(5),
        BroadcastDissemination()
    );

    return new_routing_framework_args(
        algorithm, //RoutingAlgorithm* algorithm,
        60*1000,
        60*1000,
        500,
        500,
        true
    );
}

static RoutingContext* parse_r_context(char* value, bool nested);
static ForwardingStrategy* parse_f_strategy(char* value, bool nested);
static CostMetric* parse_cost_metric(char* value, bool nested);
static AnnouncePeriod* parse_a_period(char* value, bool nested);
static DisseminationStrategy* parse_d_strategy(char* value, bool nested);

routing_framework_args* load_routing_framework_args(const char* file_path) {

    list* order = list_init();
    hash_table* configs = parse_configs_order(file_path, &order);

    if(configs == NULL) {
        char str[100];
        sprintf(str, "Config file %s not found!", file_path);
        ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
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
            } else if( strcmp(key, "r_context") == 0 ) {
                RoutingContext* new_context = parse_r_context(value, false);
                RA_setRoutingContext(args->algorithm, new_context);
            } else if( strcmp(key, "f_strategy") == 0 ) {
                ForwardingStrategy* new_f_strategy = parse_f_strategy(value, false);
                RA_setForwardingStrategy(args->algorithm, new_f_strategy);
            } else if( strcmp(key, "cost_metric") == 0 ) {
                CostMetric* new_cost_metric = parse_cost_metric(value, false);
                RA_setCostMetric(args->algorithm, new_cost_metric);
            } else if( strcmp(key, "a_period") == 0 ) {
                AnnouncePeriod* new_a_period = parse_a_period(value, false);
                RA_setAnnouncePeriod(args->algorithm, new_a_period);
            } else if( strcmp(key, "d_strategy") == 0 ) {
                DisseminationStrategy* new_d_strategy = parse_d_strategy(value, false);
                RA_setDisseminationStrategy(args->algorithm, new_d_strategy);
            } else if( strcmp(key, "seen_expiration_ms") == 0 ) {
                args->seen_expiration_ms = strtol(value, NULL, 10);
            } else if( strcmp(key, "gc_interval_s") == 0 ) {
                args->gc_interval_s = strtol(value, NULL, 10);
            } else if( strcmp(key, "max_jitter_ms") == 0  ) {
                args->max_jitter_ms = strtol(value, NULL, 10);
            } else if( strcmp(key, "period_margin_ms") == 0  ) {
                args->period_margin_ms = strtol(value, NULL, 10);
            } else if( strcmp(key, "ignore_zero_seq") == 0  ) {
                args->ignore_zero_seq = parse_bool(value);
            } else {
                char str[50];
                sprintf(str, "Unknown Config %s = %s", key, value);
                ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
            }
        } else {
            char str[50];
            sprintf(str, "Empty Config %s", key);
            ygg_log(ROUTING_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
        }

    }

    // Clean
    hash_table_delete(configs);
    list_delete(order);

    return args;
}

static RoutingContext* parse_r_context(char* value, bool nested) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

    if(strcmp(token, (name = "Static")) == 0 || strcmp(token, (name = "StaticRoutingContext")) == 0) {
        return StaticRoutingContext();
    } else if(strcmp(token, (name = "OLSR")) == 0 || strcmp(token, (name = "OLSRRoutingContext")) == 0) {
        return OLSRRoutingContext();
    } else {
        printf("Unrecognized Routing Context! \n");
        exit(-1);
    }
}

static ForwardingStrategy* parse_f_strategy(char* value, bool nested) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

    if(strcmp(token, (name = "Conventional")) == 0 || strcmp(token, (name = "ConventionalRouting")) == 0) {
        return ConventionalRouting();
    } else {
        printf("Unrecognized Forwarding Strategy! \n");
        exit(-1);
    }
}

static CostMetric* parse_cost_metric(char* value, bool nested) {
    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

    if(strcmp(token, (name = "Hops")) == 0 || strcmp(token, (name = "HopsMetric")) == 0) {
        return HopsMetric();
    } else if(strcmp(token, (name = "ETX")) == 0 || strcmp(token, (name = "ETXMetric")) == 0) {
        return ETXMetric();
    } else {
        printf("Unrecognized Cost Metric! \n");
        exit(-1);
    }
}

static AnnouncePeriod* parse_a_period(char* value, bool nested) {
    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

    if(strcmp(token, (name = "Static")) == 0 || strcmp(token, (name = "StaticAnnouncePeriod")) == 0) {

        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned long t = strtol(token, NULL, 10);

            return StaticAnnouncePeriod(t);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else {
        printf("Unrecognized Static Announce Period! \n");
        exit(-1);
    }
}

static DisseminationStrategy* parse_d_strategy(char* value, bool nested) {
    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token = NULL;

    int len = strlen(value);
    char str[len+1];

    if(!nested) {
        memcpy(str, value, len+1);
        token  = strtok_r(str, " ", &ptr);
    } else {
        token  = value;
    }

    if(strcmp(token, (name = "Broadcast")) == 0 || strcmp(token, (name = "BroadcastDissemination")) == 0) {

        return BroadcastDissemination();
    } else if(strcmp(token, (name = "Fisheye")) == 0 || strcmp(token, (name = "FisheyeDissemination")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned int n_phases = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                unsigned int phase_radius = strtol(token, NULL, 10);

                return FisheyeDissemination(n_phases, phase_radius);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if(strcmp(token, (name = "Local")) == 0 || strcmp(token, (name = "LocalDissemination")) == 0) {
        return LocalDissemination();
    } else {
        printf("Unrecognized Static Announce Period! \n");
        exit(-1);
    }
}
