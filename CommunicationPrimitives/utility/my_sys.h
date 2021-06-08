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

#ifndef SYS_H_
#define SYS_H_

#include <linux/limits.h>

#include "data_structures/hash_table.h"
#include "Yggdrasil/core/utils/utils.h"

int run_command(char* cmd, char* out, int size);

// Path
char* build_path(char* file_path, char* folder, char* file_name);

hash_table* parse_configs(const char* file_path);

hash_table* parse_configs_order(const char* file_path, list** order);

int r_mkdir(const char* path);

#endif /* SYS_H_ */
