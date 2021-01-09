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

#ifndef _ROUTING_FRAMEWORK_SOURCE_SET_H_
#define _ROUTING_FRAMEWORK_SOURCE_SET_H_

#include "utility/my_time.h"

typedef struct SourceTable_ SourceTable;
typedef struct SourceEntry_ SourceEntry;

SourceTable* newSourceTable();

void destroySourceTable(SourceTable* ss);

void SS_addEntry(SourceTable* ss, SourceEntry* entry);

SourceEntry* SS_getEntry(SourceTable* ss, unsigned char* source_id);

SourceEntry* SS_removeEntry(SourceTable* ss, unsigned char* source_id);

SourceEntry* SS_nexEntry(SourceTable* ss, void** iterator);

SourceEntry* newSourceEntry(unsigned char* source_id, unsigned short seq, struct timespec* exp_time, void* attrs);

void* destroySourceEntry(SourceEntry* se);

unsigned char* SE_getID(SourceEntry* se);

unsigned short SE_getSEQ(SourceEntry* se);

void SE_setSEQ(SourceEntry* se, unsigned short new_seq);

struct timespec* SE_getExpTime(SourceEntry* se);

void SE_setExpTime(SourceEntry* se, struct timespec* new_exp_time);

void* SE_getAttrs(SourceEntry* se);

void* SE_setAttrs(SourceEntry* se, void* new_attrs);

#endif /*_ROUTING_FRAMEWORK_SOURCE_SET_H_*/
