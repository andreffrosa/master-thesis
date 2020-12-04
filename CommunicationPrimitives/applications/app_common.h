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

#ifndef COMMUNICATION_PRIMITIVES_APP_COMMON_H_
#define COMMUNICATION_PRIMITIVES_APP_COMMON_H_

#include "data_structures/hash_table.h"

hash_table* parse_args(int argc, char* argv[]);

void unparse_host(char* hostname, unsigned int hostname_length, char* interface, unsigned int interface_length, hash_table* args);

#endif /* COMMUNICATION_PRIMITIVES_APP_COMMON_H_ */
