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

#include "my_string.h"

#include "Yggdrasil/core/utils/hashfunctions.h"

#include <string.h>

#include <assert.h>

char* str_trim(char* dest, const char* src) {

     // Left
     char* ptr = (char*) src;
     while(isspace(*ptr))
         ptr++;
     int index1 = ptr - src;

     // Right
     ptr = (char*)src + strlen(src) - 1;
     while(isspace(*ptr))
         ptr--;
     int index2 = ptr - src;
     //*(back+1) = '\0';

     int len = (index2-index1+1);
     memcpy(dest, src+index1, len);
     dest[len] = '\0';

     return dest;
 }

 unsigned long string_hash(char* str) {
 	return RSHash(str, strlen(str)); // Not safe if string has no null terminator
 }

bool equal_str(void* a, void* b) {
    char* _a = (char*) a;
    char* _b = (char*) b;
    return strcmp(_a, _b) == 0;
}

// https://stackoverflow.com/questions/32496497/standard-function-to-replace-character-or-substring-in-a-char-array/32496721
int strrplc(char *str, char orig, char rep) {
    char* ix = str;
    int n = 0;
    while((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

/*
char* center_str(char* out_str, char* in_str, unsigned int width, bool extra_right) {
     unsigned int padding = (width - strlen(in_str)) / 2;
     unsigned int extra = (width - strlen(in_str)) % 2 != 0;
     unsigned int padding_left = padding + (!extra_right ? extra : 0);
     unsigned int padding_right = padding + (extra_right ? extra : 0);
     sprintf(out_str, "%*s%s%*s", padding_left, "", in_str, padding_right, "");
     assert(strlen(out_str) == width);
     return out_str;
 }
*/



char* str_to_upper(char* str) {
    for(int i = 0; str[i] != '\0'; i++) {
        str[i] -= (str[i] >= 'a' && str[i] <= 'z')*32;
    }
    return str;
}

char* str_to_lower(char* str) {
    for(int i = 0; str[i] != '\0'; i++) {
        str[i] += (str[i] >= 'A' && str[i] <= 'Z')*32;
    }
    return str;
}

char* align_str(char* out_str, char* in_str, unsigned int width, char* type) {
    char type_[3];
    strcpy(type_, type);
    str_to_upper(type_);

    unsigned int padding = (width - strlen(in_str)) / 2;
    unsigned int extra = (width - strlen(in_str)) % 2 != 0;

    unsigned int padding_left = padding;
    unsigned int padding_right = padding + extra;

    if( strcmp(type_, "CR") == 0 ) {
        padding_left = padding + extra;
        padding_right = padding;
    } else if( strcmp(type_, "CL") == 0 ) {
        padding_left = padding;
        padding_right = padding + extra;
    } else if( strcmp(type_, "R") == 0 ) {
        padding_left = 2*padding + extra;
        padding_right = 0;
    } else if( strcmp(type_, "L") == 0 ) {
        padding_left = 0;
        padding_right = 2*padding + extra;
    } else {
        assert(false);
    }

    if( out_str == in_str ) {
        char buffer[width+1];
        strcpy(buffer, in_str);

        sprintf(out_str, "%*s%s%*s", padding_left, "", buffer, padding_right, "");
    } else {
        sprintf(out_str, "%*s%s%*s", padding_left, "", in_str, padding_right, "");
    }

    assert(strlen(out_str) == width);
    return out_str;
}

bool parse_bool(char* token) {
    assert(token);

    return strcmp(token, "True") == 0 || strcmp(token, "true") == 0;
}

char* new_str(const char* str) {
    char* str2 = malloc(strlen(str)+1);
    strcpy(str2, str);
    return str2;
}
