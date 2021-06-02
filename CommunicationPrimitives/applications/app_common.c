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
#include "utility/my_math.h"

struct option long_options[] = {
    { "app", required_argument, NULL, 'a' },
    { "discovery", required_argument, NULL, 'd' },
    { "broadcast", required_argument, NULL, 'b' },
    { "routing", required_argument, NULL, 'r' },
    { "overlay", required_argument, NULL, 'o' },
    { "interface", required_argument, NULL, 'i' },
    { "hostname", required_argument, NULL, 'h' },
    { "log", required_argument, NULL, 'l'},
    { "sleep", required_argument, NULL, 's'},
    { NULL, 0, NULL, 0 }
};

static const char* opt_str = "a:d:b:r:o:i:h:l:s:";

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
        case 'l':
        case 's':
        case 'h': {
                unsigned int str_size = (strlen(opt_arg)+1)*sizeof(char);
                char* value = malloc(str_size);
                strcpy(value, opt_arg);
                return value;
            }
        case '?':
            if ( optopt == 'a' || optopt == 'd' || optopt == 'b' || optopt == 'r' || optopt == 'o' || optopt == 'i' || optopt == 'h' || optopt == 'l'|| optopt == 's')
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
        //const char* h = getHostname();
        FILE* f = fopen("/etc/hostname","r");
    	if(f > 0) {
    		struct stat s;
    		if(fstat(fileno(f), &s) < 0){
    			perror("FSTAT");
    		}

            assert(s.st_size+1 <= hostname_length);

    		memset(hostname, 0, s.st_size+1);
    		if(fread(hostname, 1, s.st_size, f) <= 0){
    			if(s.st_size < 8) {
    				//free(hostname);
    				//hostname = malloc(9);
    				memset(hostname,0,9);
    			}
    			int r = rand() % 10000;
    			sprintf(hostname, "host%04d", r);
    		} else{

    			int i;
    			for(i = 0; i < s.st_size + 1; i++){
    				if(hostname[i] == '\n') {
    					hostname[i] = '\0';
    					break;
    				}
    			}
    		}

    	} else {

    		//hostname = malloc(9);
    		memset(hostname,0,9);

    		int r = rand() % 10000;
    		sprintf(hostname, "host%04d", r);
    	}
        /*if(h && strcmp(h, "") != 0) {
            assert(strlen(h) < hostname_length);
            strcpy(hostname, h);
        } else {
            assert(strlen("hostname") < hostname_length);
            strcpy(hostname, "hostname");
        }*/
    }
}

unsigned long getNextDelay(const char* type, unsigned long elapsed_ms) {
    unsigned long t = 0L;

    char aux[strlen(type)+1];
    strcpy(aux, type);

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

                                token = strtok_r(NULL, " ", &ptr);
                        		if(token != NULL) {
                                    unsigned long duration_s = strtol(token, NULL, 10);

                                    double seconds = elapsed_ms / 1000.0;

                                    double aux = dev + stretching*(seconds / duration_s);
                                    double f = (1.0 + cos(2.0*M_PI*aux))/2.0;

                                    lambda = min_lambda + (max_lambda - min_lambda)*f;
                                    //printf("lambda = %f seconds = %f\n", lambda, seconds);
                        		} else {
                        			printf("Parameter 5 of %s not passed!\n", name);
                        			exit(-1);
                        		}
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

        t = lround(randomExponential(lambda/1000.0)); // t is in millis, lambda is per second
    } else if(strcmp(token, (name="Periodic")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double prob = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                unsigned long periodic_timer_s = strtol(token, NULL, 10);

                // Compute how many times should not send to accumulate in a single timer
                int counter = 1;
                while( prob < 1.0 && getRandomProb() > prob ) {
                    counter++;
                }

                t = counter*periodic_timer_s*1000;
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    }

    //printf("getNextDelay t = %lu\n", t);

    return t;
}
