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

#include "source_table.h"

#include "data_structures/hash_table.h"

#include "utility/my_misc.h"

#include <uuid/uuid.h>
#include <assert.h>

typedef struct SourceTable_ {
    hash_table* ht;
} SourceTable;

typedef struct SourceEntry_ {
    uuid_t source_id;
    unsigned short seq;
    struct timespec exp_time;
    void* attrs;
} SourceEntry;

SourceTable* newSourceTable() {
    SourceTable* ss = malloc(sizeof(SourceTable));

    ss->ht = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);

    return ss;
}

static void delete_source_item(hash_table_item* hit, void* aux) {
    destroySourceEntry((SourceEntry*)hit->value);
    free(hit);
}

void destroySourceTable(SourceTable* ss) {
    if(ss) {
        hash_table_delete_custom(ss->ht, &delete_source_item, NULL);
        free(ss);
    }
}

void SS_addEntry(SourceTable* ss, SourceEntry* entry) {
    assert(ss && entry);

    void* old = hash_table_insert(ss->ht, entry->source_id, entry);
    assert(old == NULL);
}

SourceEntry* SS_getEntry(SourceTable* ss, unsigned char* source_id) {
    assert(ss && source_id);

    return (SourceEntry*)hash_table_find_value(ss->ht, source_id);
}

SourceEntry* SS_removeEntry(SourceTable* ss, unsigned char* source_id) {
    assert(ss && source_id);

    hash_table_item* it = hash_table_remove_item(ss->ht, source_id);
    if(it) {
        SourceEntry* entry = (SourceEntry*)it->value;
        free(it);
        return entry;
    } else {
        return NULL;
    }
}

SourceEntry* SS_nexEntry(SourceTable* ss, void** iterator) {
    assert(ss);

    hash_table_item* item = hash_table_iterator_next(ss->ht, iterator);
    if(item) {
        return (SourceEntry*)(item->value);
    } else {
        return NULL;
    }
}

SourceEntry* newSourceEntry(unsigned char* source_id, unsigned short seq, struct timespec* exp_time, void* attrs) {
    assert(source_id && exp_time);

    SourceEntry* se = malloc(sizeof(SourceEntry));

    uuid_copy(se->source_id, source_id);
    se->seq = seq;
    copy_timespec(&se->exp_time, exp_time);
    se->attrs = attrs;

    return se;
}

void* destroySourceEntry(SourceEntry* se) {
    if(se) {
        void* attrs = se->attrs;
        free(se);
        return attrs;
    }

    return NULL;
}

unsigned char* SE_getID(SourceEntry* se) {
    assert(se);
    return se->source_id;
}

unsigned short SE_getSEQ(SourceEntry* se) {
    assert(se);
    return se->seq;
}

void SE_setSEQ(SourceEntry* se, unsigned short new_seq) {
    assert(se);
    se->seq = new_seq;
}

struct timespec* SE_getExpTime(SourceEntry* se) {
    assert(se);
    return &se->exp_time;
}

void SE_setExpTime(SourceEntry* se, struct timespec* new_exp_time) {
    assert(se && new_exp_time);
    copy_timespec(&se->exp_time, new_exp_time);
}

void* SE_getAttrs(SourceEntry* se) {
    assert(se);
    return se->attrs;
}

void* SE_setAttrs(SourceEntry* se, void* new_attrs) {
    assert(se);
    void* old = se->attrs;
    se->attrs = new_attrs;
    return old;
}
