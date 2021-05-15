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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uuid/uuid.h>

#include "data_structures/list.h"

#include "utility/my_misc.h"

int main(int argc, char* argv[]) {

    list* l = list_init();

    for(int i = 0; i < 10; i++) {
        int* n = new_int(i);
        list_add_item_to_tail(l, n);
    }

    list_shuffle(l, 3);
    printf("[");
    for(list_item* it = l->head; it; it = it->next) {
        printf("%d ", *((int*)it->data));
    }
    printf("]\n");

    list_delete(l);

    return 0;
}
