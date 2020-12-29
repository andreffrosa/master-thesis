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

#include "seen_messages.h"

#include "data_structures/hash_table.h"
#include "utility/my_time.h"
#include "utility/my_misc.h"

#include <assert.h>

 typedef struct _SeenMessages {
     hash_table* seen_messages;
 } SeenMessages;

SeenMessages* newSeenMessages() {
    SeenMessages* m = malloc(sizeof(SeenMessages));
    m->seen_messages = hash_table_init((hashing_function)&uuid_hash, (comparator_function)&equalID);
    return m;
}

void destroySeenMessages(SeenMessages* m) {
    hash_table_delete(m->seen_messages);
    free(m);
}

bool SeenMessagesContains(SeenMessages* m, unsigned char* id) {
    return hash_table_find_value(m->seen_messages, id) != NULL;
}

void SeenMessagesAdd(SeenMessages* m, unsigned char* id, struct timespec* current_time) {
    struct timespec* t = malloc(sizeof(struct timespec));
    memcpy(t, current_time, sizeof(struct timespec));

    unsigned char* key = malloc(sizeof(uuid_t));
    uuid_copy(key, id);

    void* old = hash_table_insert(m->seen_messages, key, t);
    assert(old == NULL);
}

unsigned int SeenMessagesGC(SeenMessages* m, struct timespec* current_time, struct timespec* seen_expiration) {
    struct timespec exp_time = {0};
    subtract_timespec(&exp_time, current_time, seen_expiration);

    list* to_delete = list_init();

    for(int i = 0; i < m->seen_messages->array_size; i++) {
        double_list* l = m->seen_messages->array[i];
        for(double_list_item* it = l->head; it; it = it->next) {
            hash_table_item* hit = (hash_table_item*)(it->data);

            struct timespec* t = (struct timespec*)(hit->value);
            if(compare_timespec(t, &exp_time) < 0) {
                unsigned char* aux = malloc(sizeof(uuid_t));
                uuid_copy(aux, hit->key);
                list_add_item_to_tail(to_delete, aux);
            }
        }
    }

    unsigned deleted = to_delete->size;

    void* v = NULL;
	while( (v = list_remove_head(to_delete)) ) {
        hash_table_item* hit = (hash_table_item*)hash_table_remove(m->seen_messages, v);
        free(hit->key);
        free(hit->value);
        free(hit);
        free(v);
    }
	free(to_delete);

    return deleted;
}
