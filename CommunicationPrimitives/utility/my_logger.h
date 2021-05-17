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
 * (C) 2021
 *********************************************************/

#ifndef _MY_LOGGER_H_
#define _MY_LOGGER_H_

#include <stdio.h>

typedef struct MyLogger_ MyLogger;

MyLogger* new_my_logger(FILE* out, const char* hostname);

void my_logger_write(MyLogger* logger, char* proto, char* event, char* desc);

void my_logger_flush(MyLogger* logger);

void my_logger_close(MyLogger* logger);

#endif /* _MY_LOGGER_H_ */
