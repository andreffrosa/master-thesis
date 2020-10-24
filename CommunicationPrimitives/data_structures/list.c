/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Authors:
 * André Rosa (af.rosa@campus.fct.unl.pt)
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2019
 *********************************************************/

#include "list.h"

#include <assert.h>

void list_append(list* l1, list* l2) {

    assert(l1 != NULL);
    assert(l2 != NULL);

    if(l2->size == 0) {
        ; // Do nothing to l1
    } else if(l1->size == 0) {
        l1->head = l2->head;
        l1->tail = l2->tail;
        l1->size = l2->size;
    } else {
        l1->tail->next = l2->head;
        l1->tail = l2->tail;
        l1->size += l2->size;
    }

	free(l2);
}

void list_delete(list* l) {
	void* it = NULL;
	while( (it = list_remove_head(l)) )
		free(it);
	free(l);
}

void list_delete_keep(list* l) {
	void* it = NULL;
	while( (it = list_remove_head(l)) )
		; //free(it);
	free(l);
}


bool list_contained(list* l1, list* l2, comparator_function cmp, bool orEqual) {
    bool contained = true;

    for(list_item* it = l1->head; it; it = it->next) {
        bool found = false;
        for(list_item* it2 = l2->head; it2; it2 = it2->next) {
            if(cmp(it->data, it2->data)) {
                found = true;
                break;
            }
        }
        if(!found) {
            contained = false;
            break;
        }
    }

    if(!orEqual) {
        // If is contained and they are the same size, then they are equal
        return contained && (l1->size < l2->size);
    } else {
        return contained;
    }
}

bool list_equal(list* l1, list* l2, comparator_function cmp) {
    return (l1->size == l2->size) && list_contained(l1, l2, cmp, true);
}

bool list_is_empty(list* l) {
    return l->size == 0;
}

list* list_clone(list* l, unsigned int data_size) {
    list* l2 = list_init();

    for(list_item* it = l->head; it; it = it->next) {
        void* x = malloc(data_size);
        memcpy(x, it->data, data_size);
        list_add_item_to_tail(l2, x);
    }

    return l2;
}

list* list_intercept(list* l1, list* l2, comparator_function cmp, unsigned int data_size) {
    list* result = list_init();

    for(list_item* it = l1->head; it; it = it->next) {
        void* v = list_find_item(l2, cmp, it->data);
        if(v) {
            void* x = malloc(data_size);
            memcpy(x, v, data_size);
            list_add_item_to_tail(result, x);
        }
    }

    return result;
}

list* list_difference(list* l1, list* l2, comparator_function cmp, unsigned int data_size) {
    list* result = list_init();

    for(list_item* it = l1->head; it; it = it->next) {
        void* v = list_find_item(l2, cmp, it->data);
        if(v == NULL) {
            void* x = malloc(data_size);
            memcpy(x, it->data, data_size);
            list_add_item_to_tail(result, x);
        }
    }

    return result;
}

list* list_map(list* l, void* (*f)(void* v, unsigned int argc, void** argv), unsigned int argc, void** argv) {
    list* result = list_init();

    for(list_item* it = l->head; it; it = it->next) {
        list_add_item_to_tail(result, f(it->data, argc, argv));
    }

    return result;
}
