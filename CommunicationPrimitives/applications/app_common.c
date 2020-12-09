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

#include "app_common.h"

#include <stdlib.h>
#include <getopt.h>
#include <linux/limits.h>

#include <limits.h>

#include <assert.h>

#include "Yggdrasil.h"

#include "utility/my_sys.h"
#include "utility/my_time.h"
#include "utility/my_misc.h"
#include "utility/my_string.h"

struct option long_options[] = {
    { "app", required_argument, NULL, 'a' },
    { "discovery", required_argument, NULL, 'd' },
    { "broadcast", required_argument, NULL, 'b' },
    { "routing", required_argument, NULL, 'r' },
    { "overlay", required_argument, NULL, 'o' },
    { "interface", required_argument, NULL, 'i' },
    { "hostname", required_argument, NULL, 'h' },
    { NULL, 0, NULL, 0 }
};

static const char* opt_str = "a:d:b:r:o:i:h:";

static void* process_arg(char option, int option_index, char* args);

hash_table* parse_args(int argc, char* argv[]) {

    hash_table* args = hash_table_init((hashing_function) &string_hash, (comparator_function) &equal_str);

    int option_index = -1;
    int ch;
    opterr = 0;
    while ((ch = getopt_long(argc, argv, opt_str, long_options, &option_index)) != -1) {
        if(option_index == -1) {
            bool found = false;
            for(int i = 0; long_options[i].name != NULL; i++) {
                if(ch == long_options[i].val) {
                    option_index = i;
                    found = true;
                    break;
                }
            }
            if(!found) {
                printf("Unrecognized ch %c\n", ch);
            }
        }

        void* value = process_arg(ch, option_index, optarg);
        if(value) {
            unsigned int str_size = (strlen(long_options[option_index].name)+1)*sizeof(char);
            char* key = malloc(str_size);
            strcpy(key, long_options[option_index].name);

            hash_table_insert(args, key, value);
        }

        option_index = -1;
    }

    return args;
}

static void* process_arg(char option, int option_index, char* opt_arg) {

    switch (option) {
        case 'a':
        case 'd':
        case 'b':
        case 'r':
        case 'o':
        case 'i':
        case 'h': {
                unsigned int str_size = (strlen(opt_arg)+1)*sizeof(char);
                char* value = malloc(str_size);
                strcpy(value, opt_arg);
                return value;
            }
        case '?':
            if ( optopt == 'a' || optopt == 'd' || optopt == 'b' || optopt == 'r' || optopt == 'o' || optopt == 'i' || optopt == 'h')
                fprintf(stderr, "Option -%c is followed by a parameter.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
        default: return NULL;
    }

}

void unparse_host(char* hostname, unsigned int hostname_length, char* interface, unsigned int interface_length, hash_table* args) {
    assert(hostname && interface);

    // Interface
    char* interface_arg = hash_table_find_value(args, "interface");
    if(interface_arg) {
        assert(strlen(interface_arg) < interface_length);
        strcpy(interface, interface_arg);
    } else {
        assert(strlen("wlan0") < interface_length);
        strcpy(interface, "wlan0");
    }

    // Hostname
    char* hostname_arg = hash_table_find_value(args, "hostname");
    if(hostname_arg) {
        assert(strlen(hostname_arg) < hostname_length);
        strcpy(hostname, hostname_arg);
    } else {
        const char* h = getHostname();
        if(h && strcmp(h, "") != 0) {
            assert(strlen(h) < hostname_length);
            strcpy(hostname, h);
        } else {
            assert(strlen("hostname") < hostname_length);
            strcpy(hostname, "hostname");
        }
    }
}
