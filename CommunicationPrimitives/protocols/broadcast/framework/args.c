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

#include <assert.h>

broadcast_framework_args* new_broadcast_framework_args(BroadcastAlgorithm* algorithm, unsigned long seen_expiration_ms, unsigned long gc_interval_s, bool late_delivery) {
    broadcast_framework_args* args = malloc(sizeof(broadcast_framework_args));

    args->algorithm = algorithm;
    args->seen_expiration_ms = seen_expiration_ms;
    args->gc_interval_s = gc_interval_s;
    args->late_delivery = late_delivery;

    return args;
}

broadcast_framework_args* default_broadcast_framework_args() {
    return new_broadcast_framework_args(Flooding(500), 1*60*1000, 3*60, false);
}

static BroadcastAlgorithm* parse_broadcast_algorithm(char* value);
static RetransmissionPolicy* parse_r_policy(char* value);
static RetransmissionDelay* parse_r_delay(char* value);
static RetransmissionContext* parse_r_context(char* value);

broadcast_framework_args* load_broadcast_framework_args(const char* file_path) {
    list* order = list_init();
    hash_table* configs = parse_configs_order(file_path, &order);

    if(configs == NULL) {
        char str[100];
        sprintf(str, "Config file %s not found!", file_path);
        ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
        ygg_logflush();

        exit(-1);
    }

    broadcast_framework_args* args = default_broadcast_framework_args();

    for(list_item* it = order->head; it; it = it->next) {
        char* key = (char*)it->data;
        char* value = (char*)hash_table_find_value(configs, key);

        if( value != NULL ) {
            if( strcmp(key, "algorithm") == 0 ) {
                destroyBroadcastAlgorithm(args->algorithm);
                args->algorithm = parse_broadcast_algorithm(value);
            } else if( strcmp(key, "r_policy") == 0 ) {
                RetransmissionPolicy* new_r_policy = parse_r_policy(value);
                BA_setRetransmissionPolicy(args->algorithm, new_r_policy);
            } else if( strcmp(key, "r_delay") == 0 ) {
                RetransmissionDelay* new_r_delay = parse_r_delay(value);
                BA_setRetransmissionDelay(args->algorithm, new_r_delay);
            } else if( strcmp(key, "r_context") == 0 ) {
                RetransmissionContext* new_r_context = parse_r_context(value);
                BA_setRetransmissionContext(args->algorithm, new_r_context);
            } else if( strcmp(key, "r_phases") == 0 ) {
                unsigned int new_r_phases = strtol(value, NULL, 10);
                BA_setRetransmissionPhases(args->algorithm, new_r_phases);
            } else if( strcmp(key, "seen_expiration_ms") == 0 ) {
                args->seen_expiration_ms = strtol(value, NULL, 10);
            } else if( strcmp(key, "gc_interval_s") == 0 ) {
                args->gc_interval_s = strtol(value, NULL, 10);
            } else if( strcmp(key, "late_delivery") == 0 ) {
                args->late_delivery = strcmp(value, "false") == 0 ? false : true;
            } else {
                char str[50];
                sprintf(str, "Unknown Config %s = %s", key, value);
                ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
            }
        } else {
            char str[50];
            sprintf(str, "Empty Config %s", key);
            ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "ARG ERROR", str);
        }
    }

    // Clean
    hash_table_delete(configs);
    list_delete(order);

    return args;
}

static BroadcastAlgorithm* parse_broadcast_algorithm(char* value) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token  = strtok_r(value, " ", &ptr);

    if(strcmp(token, (name = "Flooding")) == 0) {

        token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			return Flooding(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
    } else if(strcmp(token, (name = "Gossip1")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double p = strtod (token, NULL);

				return Gossip1(t, p);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip1_hops")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double p = strtod (token, NULL);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					unsigned int k = strtol(token, NULL, 10);

					return Gossip1_hops(t, p, k);
				} else {
					printf("Parameter 3 of %s not passed!\n", name);
					exit(-1);
				}
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip2")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double p1 = strtod (token, NULL);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					unsigned int k = strtol(token, NULL, 10);

					token = strtok_r(NULL, " ", &ptr);
					if(token != NULL) {
						double p2 = strtod (token, NULL);

						token = strtok_r(NULL, " ", &ptr);
						if(token != NULL) {
							unsigned int n = strtol(token, NULL, 10);

							return Gossip2(t, p1, k, p2, n);
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
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip3")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t1 = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned long t2 = strtol(token, NULL, 10);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					double p = strtod (token, NULL);

					token = strtok_r(NULL, " ", &ptr);
					if(token != NULL) {
						unsigned int k = strtol(token, NULL, 10);

						token = strtok_r(NULL, " ", &ptr);
						if(token != NULL) {
							unsigned int m = strtol(token, NULL, 10);

							return Gossip3(t1, t2, p, k, m);
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
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Rapid")) == 0 || strcmp(token, (name = "RAPID")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				double beta = strtod(token, NULL);

				return RAPID(t, beta);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "EnhancedRapid")) == 0 || strcmp(token, (name = "EnhancedRAPID")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t1 = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned long t2 = strtol(token, NULL, 10);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					double beta = strtod(token, NULL);

					return EnhancedRAPID(t1, t2, beta);
				} else {
					printf("Parameter 3 of %s not passed!\n", name);
					exit(-1);
				}
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Counting")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned int c = strtol(token, NULL, 10);

				return Counting(t, c);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "HopCountAided")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			return HopCountAided(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA1")) == 0 || strcmp(token, (name = "CountingNABA")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned int c = strtol(token, NULL, 10);

				return NABA1(t, c);
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA2")) == 0 || strcmp(token, (name = "PbCountingNABA")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			token = strtok_r(NULL, " ", &ptr);
			if(token != NULL) {
				unsigned int c1 = strtol(token, NULL, 10);

				token = strtok_r(NULL, " ", &ptr);
				if(token != NULL) {
					unsigned int c2 = strtol(token, NULL, 10);

					return NABA2(t, c1, c2);
				} else {
					printf("Parameter 3 of %s not passed!\n", name);
					exit(-1);
				}
			} else {
				printf("Parameter 2 of %s not passed!\n", name);
				exit(-1);
			}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	}
    else if(strcmp(token, (name = "NABA3")) == 0 || strcmp(token, (name = "Naba3")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return NABA3(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA4")) == 0 || strcmp(token, (name = "Naba4")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

			return NABA4(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NABA3e4")) == 0 || strcmp(token, (name = "Naba3e4")) == 0 || strcmp(token, (name = "CriticalNABA")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                unsigned int np = strtol(token, NULL, 10);

                return NABA3e4(t, np);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	}
    else if(strcmp(token, (name = "SBA")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return SBA(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "LENWB")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return LENWB(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "MPR")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return MPR(t);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "AHBP")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			int ex = (int) strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned long t = strtol(token, NULL, 10);

                unsigned int route_max_len = (unsigned int)strtol(token, NULL, 10);

                return AHBP(ex, t, route_max_len);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "DynamicProbability")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			double p_l = strtod(token, NULL);

                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
        			double p_u = strtod(token, NULL);

                    token = strtok_r(NULL, " ", &ptr);
            		if(token != NULL) {
            			double d = strtod(token, NULL);

                        token = strtok_r(NULL, " ", &ptr);
                		if(token != NULL) {
                			unsigned long t1 = strtol(token, NULL, 10);

                            token = strtok_r(NULL, " ", &ptr);
                    		if(token != NULL) {
                    			unsigned long t2 = strtol(token, NULL, 10);

                                return DynamicProbability(p, p_l, p_u, d, t1, t2);
                    		} else {
                    			printf("Parameter 6 of %s not passed!\n", name);
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
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if(strcmp(token, (name = "RADExtension")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long delta_t = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int c = strtol(token, NULL, 10);

                return RADExtension(delta_t, c);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if(strcmp(token, (name = "HopCountAwareRADExtension")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long delta_t = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int c = strtol(token, NULL, 10);

                return HopCountAwareRADExtension(delta_t, c);
            } else {
                printf("Parameter 2 of %s not passed!\n", name);
                exit(-1);
            }
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else {
		printf("Unrecognized Broadcast Algorithm! \n");
		exit(-1);
	}

}

static RetransmissionPolicy* parse_r_policy(char* value) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token  = strtok_r(value, " ", &ptr);

    if(strcmp(token, (name = "True")) == 0 || strcmp(token, (name = "AlwaysTrue")) == 0 || strcmp(token, (name = "TruePolicy")) == 0 ) {
        return TruePolicy();
	} else if(strcmp(token, (name = "Probability")) == 0 || strcmp(token, (name = "ProbabilityPolicy")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p = strtod (token, NULL);

            return ProbabilityPolicy(p);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Count")) == 0 || strcmp(token, (name = "CountPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned int c = strtol(token, NULL, 10);

            return CountPolicy(c);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "NeighborCounting")) == 0 || strcmp(token, (name = "NeighborCountingPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned int c = strtol(token, NULL, 10);

            return NeighborCountingPolicy(c);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "PbNeighCounting")) == 0 || strcmp(token, (name = "PbNeighCountingPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned int c1 = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int c2 = strtol(token, NULL, 10);

                return PbNeighCountingPolicy(c1, c2);
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "HorizonProbability")) == 0 || strcmp(token, (name = "HorizonProbabilityPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int k = strtol(token, NULL, 10);

                return HorizonProbabilityPolicy(p, k);
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Gossip2")) == 0 || strcmp(token, (name = "Gossip2Policy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p1 = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int k = strtol(token, NULL, 10);

                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
        			double p2 = strtod(token, NULL);

                    token = strtok_r(NULL, " ", &ptr);
            		if(token != NULL) {
            			unsigned int n = strtol(token, NULL, 10);

                        return Gossip2Policy(p1, k, p2, n);
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
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Rapid")) == 0 || strcmp(token, (name = "RapidPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double beta = strtod(token, NULL);

            return RapidPolicy(beta);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "EnhancedRapid")) == 0 || strcmp(token, (name = "EnhancedRapidPolicy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double beta = strtod(token, NULL);

            return EnhancedRapidPolicy(beta);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
    } else if(strcmp(token, (name = "Gossip3")) == 0 || strcmp(token, (name = "Gossip3Policy")) == 0) {
		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			double p = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned int k = strtol(token, NULL, 10);

                token = strtok_r(NULL, " ", &ptr);
        		if(token != NULL) {
        			unsigned int m = strtol(token, NULL, 10);

                    return Gossip3Policy(p, k, m);
        		} else {
        			printf("Parameter 3 of %s not passed!\n", name);
        			exit(-1);
        		}
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "HopCountAided")) == 0 || strcmp(token, (name = "HopCountAidedPolicy")) == 0) {
        return HopCountAidedPolicy();
    } else if(strcmp(token, (name = "SBA")) == 0 || strcmp(token, (name = "SBAPolicy")) == 0) {
        return SBAPolicy();
    } else if(strcmp(token, (name = "LENWB")) == 0 || strcmp(token, (name = "LENWBPolicy")) == 0) {
        return LENWBPolicy();
    } else if(strcmp(token, (name = "DelegatedNeighbors")) == 0 || strcmp(token, (name = "DelegatedNeighborsPolicy")) == 0) {
        return DelegatedNeighborsPolicy();
    } else if(strcmp(token, (name = "CriticalNeigh")) == 0 || strcmp(token, (name = "CriticalNeighPolicy")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double min_critical_coverage = strtod(token, NULL);

            return CriticalNeighPolicy(true, false, min_critical_coverage);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if(strcmp(token, (name = "AHBP")) == 0 || strcmp(token, (name = "AHBPPolicy")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            int ex = strtol(token, NULL, 10);

            return AHBPPolicy(ex);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
    } else if(strcmp(token, (name = "DynamicProbability")) == 0 || strcmp(token, (name = "DynamicProbabilityPolicy")) == 0) {
        return DynamicProbabilityPolicy();
    } else {
        printf("Unrecognized Retransmission Policy! \n");
		exit(-1);
	}

}

static RetransmissionDelay* parse_r_delay(char* value) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token  = strtok_r(value, " ", &ptr);

    if(strcmp(token, (name = "Random")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return RandomDelay(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "DensityNeigh")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return DensityNeighDelay(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "SBADelay")) == 0) {


		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t = strtol(token, NULL, 10);

            return SBADelay(t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "TwoPhaseRandomDelay")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long t1 = strtol(token, NULL, 10);

            token = strtok_r(NULL, " ", &ptr);
    		if(token != NULL) {
    			unsigned long t2 = strtol(token, NULL, 10);

                return TwoPhaseRandomDelay(t1, t2);
    		} else {
    			printf("Parameter 2 of %s not passed!\n", name);
    			exit(-1);
    		}
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "Null")) == 0 || strcmp(token, (name = "NullDelay")) == 0) {
        return NullDelay();
	} else if(strcmp(token, (name = "RADExtensionDelay")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long delta_t = strtol(token, NULL, 10);

            return RADExtensionDelay(delta_t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else if(strcmp(token, (name = "HopCountAwareRADExtensionDelay")) == 0) {

		token = strtok_r(NULL, " ", &ptr);
		if(token != NULL) {
			unsigned long delta_t = strtol(token, NULL, 10);

            return HopCountAwareRADExtensionDelay(delta_t);
		} else {
			printf("Parameter 1 of %s not passed!\n", name);
			exit(-1);
		}
	} else {
		exit(-1);
		printf("Unrecognized R. Delay! \n");
	}
}

static RetransmissionContext* parse_r_context(char* value) {

    if(value == NULL) {
        printf("No parameter passed");
        exit(-1);
    }

    char* name = NULL;
    char* ptr = NULL;
    char* token  = strtok_r(value, " ", &ptr);

    if(strcmp(token, (name = "Empty")) == 0 || strcmp(token, (name = "EmptyContext")) == 0) {
        return EmptyContext();
	} else if(strcmp(token, (name = "Hops")) == 0 || strcmp(token, (name = "HopsContext")) == 0) {
		return HopsContext();
	} else if(strcmp(token, (name = "Parents")) == 0 || strcmp(token, (name = "ParentsContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned int max_len = strtol(token, NULL, 10);
    		return ParentsContext(max_len);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "Neighbors")) == 0 || strcmp(token, (name = "NeighborsContext")) == 0) {
        return NeighborsContext();
	} else if(strcmp(token, (name = "MultiPointRelay")) == 0 || strcmp(token, (name = "MultiPointRelayContext")) == 0) {
        return MultiPointRelayContext(NeighborsContext());
	} else if(strcmp(token, (name = "AHBP")) == 0 || strcmp(token, (name = "AHBPContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned int max_len = strtol(token, NULL, 10);

            return AHBPContext(NeighborsContext(), RouteContext(max_len));
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "Route")) == 0 || strcmp(token, (name = "RouteContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned int max_len = strtol(token, NULL, 10);

            return RouteContext(max_len);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "DynamicProbability")) == 0 || strcmp(token, (name = "DynamicProbabilityContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            double p = strtod(token, NULL);

            token = strtok_r(NULL, " ", &ptr);
            if(token != NULL) {
                double p_l = strtod(token, NULL);

                token = strtok_r(NULL, " ", &ptr);
                if(token != NULL) {
                    double p_u = strtod(token, NULL);

                    token = strtok_r(NULL, " ", &ptr);
                    if(token != NULL) {
                        double d = strtod(token, NULL);

                        token = strtok_r(NULL, " ", &ptr);
                        if(token != NULL) {
                            unsigned long t = strtol(token, NULL, 10);

                            return DynamicProbabilityContext(p, p_l, p_u, d, t);
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
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "LabelNeighs")) == 0 || strcmp(token, (name = "LabelNeighsContext")) == 0) {
        return LabelNeighsContext(NeighborsContext());
	} else if(strcmp(token, (name = "HopCountAwareRADExtension")) == 0 || strcmp(token, (name = "HopCountAwareRADExtensionContext")) == 0) {
        token = strtok_r(NULL, " ", &ptr);
        if(token != NULL) {
            unsigned long delta_t = strtol(token, NULL, 10);

            return HopCountAwareRADExtensionContext(delta_t);
        } else {
            printf("Parameter 1 of %s not passed!\n", name);
            exit(-1);
        }
	} else if(strcmp(token, (name = "LENWB")) == 0 || strcmp(token, (name = "LENWBContext")) == 0) {
        return LENWBContext(NeighborsContext());
	} else if(strcmp(token, (name = "Latency")) == 0 || strcmp(token, (name = "LatencyContext")) == 0) {
        return LatencyContext();
	} else {
        printf("Unrecognized Retransmission Context! \n");
		exit(-1);
	}
}
