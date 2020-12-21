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

#ifndef MY_STRING_H_
#define MY_STRING_H_

#include "Yggdrasil/core/utils/utils.h"

char* str_trim(char* dest, const char* src);

unsigned long string_hash(char* str);

bool equal_str(void* a, void* b);

int strrplc(char *str, char orig, char rep);

// char* center_str(char* out_str, char* in_str, unsigned int width, bool extra_right);

char* str_to_upper(char* str);

char* str_to_lower(char* str);

char* align_str(char* out_str, char* in_str, unsigned int width, char* type);

bool parse_bool(char* token);

#endif /* MY_STRING_H_ */
