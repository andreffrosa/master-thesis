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

#include "olsr_utils.h"

#include <assert.h>

#include "utility/my_misc.h"

bool equalN2Tuple(void* a, void* b) {
    N2_Tuple* x = a, *y = b;
	return uuid_compare(x->id, y->id) == 0;
}

N1_Tuple* newN1Tuple(unsigned char* id, double d1, double w, list* ns) {
    N1_Tuple* x = malloc(sizeof(N1_Tuple));

    uuid_copy(x->id, id);
    x->d1 = d1;
    x->w = w;
    x->ns = ns;

    return x;
}

N2_Tuple* newN2Tuple(unsigned char* id, double d2) {
    N2_Tuple* x = malloc(sizeof(N2_Tuple));

    uuid_copy(x->id, id);
    x->d2 = d2;

    return x;
}

static void delete_n1_tuple(hash_table_item* hit, void* args) {
    N1_Tuple* x = (N1_Tuple*)hit->value;

    if(x->ns)
        list_delete(x->ns);
    free(x);
}

void destroyN1(hash_table* n1) {
    if( n1 ) {
        hash_table_delete_custom(n1, &delete_n1_tuple, NULL);
    }
}

void destroyN2(list* n2) {
    if( n2 ) {
        list_delete(n2);
    }
}

static list* compute_n(hash_table* n1, list* n2) {
    list* n = list_init();

    N1_Tuple* n1_tuple = NULL;
    unsigned char* n2_neigh_id = NULL;
    for( list_item* it = n2->head; it; it = it->next ) {
        n2_neigh_id = it->data;
        n1_tuple = hash_table_find_value(n1, n2_neigh_id);

        // If strict two hop
        if( n1_tuple == NULL ) {
            unsigned char* x = malloc(sizeof(uuid_t));
            uuid_copy(x, n2_neigh_id);
            list_add_item_to_tail(n, x);
        }

        // Is also 1-hop neighbor
        else {
            void* iterator = NULL;
            N1_Tuple* current_n1_tuple = NULL;
            hash_table_item* hit = NULL;
            while( (hit = hash_table_iterator_next(n1, &iterator)) ) {
                current_n1_tuple = hit->value;

                N2_Tuple* n2_tuple = list_find_item(current_n1_tuple->ns, &equalN2Tuple, n2_neigh_id);
                if( n2_tuple != NULL ) {

                    // Exists a better path through other neighbor
                    if( n2_tuple->d2 > n1_tuple->d1 ) {
                        unsigned char* x = malloc(sizeof(uuid_t));
                        uuid_copy(x, n2_neigh_id);
                        list_add_item_to_tail(n, x);
                    }
                }
            }
        }
    }

    return n;
}

/*
R(x,M):
  For an element x in N1, the number of elements y in N for which
  d(x,y) is defined has maximal value among the d(z,y) for all z in
  N1 and no such maximal values have z in M (there is no selected maximal node yet).  (Note that, denoting
  the empty set by 0, D(x) = R(x,0).)
*/

static unsigned int compute_r(hash_table* n1, list* m, list* n, N1_Tuple* x) {
    //list* r = list_init();

    unsigned int counter = 0;

    for( list_item* it = x->ns->head; it; it = it->next ) {
        N2_Tuple* n2_tuple = it->data;

        for( list_item* it2 = n->head; it2; it2 = it2->next ) {
            unsigned char* n2_neigh_id = it->data;

            if( uuid_compare(n2_tuple->id, n2_neigh_id) == 0 ) { // if d(x,y) is defined
                double max_value = 0.0;
                bool found = false;

                void* iterator = NULL;
                N1_Tuple* z = NULL;
                hash_table_item* hit = NULL;
                while( (hit = hash_table_iterator_next(n1, &iterator)) ) {
                    z = hit->value;

                    N2_Tuple* n2_tuple_2 = list_find_item(z->ns, &equalN2Tuple, n2_neigh_id);
                    if( n2_tuple_2 != NULL ) {
                        if( n2_tuple_2->d2 >= max_value ) {
                            if( list_find_item(m, &equalID, z->id) == NULL ) { // There is no selected maximal node yet
                                max_value = n2_tuple_2->d2;
                                found = true;
                            }
                        }
                    }
                }

                if( n2_tuple->d2 == max_value && found ) {
                    counter++;
                }
            }
        }
    }

    return counter;
    //return r->size;
}

static unsigned int compute_d(hash_table* n1, list* n, N1_Tuple* x) {
    return compute_r(n1, list_init(), n, x);
}

static list* compute_not_covered(hash_table* n1, list* m, list* n) {
    list* s = list_init();

    void* iterator = NULL;
    N1_Tuple* x = NULL;
    hash_table_item* hit = NULL;
    while( (hit = hash_table_iterator_next(n1, &iterator)) ) {
        x = hit->value;

        if( compute_r(n1, m, n, x) > 0 ) {
            unsigned char* id = malloc(sizeof(uuid_t));
            uuid_copy(id, x->id);
            list_add_item_to_tail(s, id);
        }
    }

    return s;
}

list* compute_multipoint_relays(hash_table* n1, list* n2, list* initial) {

    list* m = initial ? list_clone(initial, sizeof(uuid_t)) : list_init();

    // TODO: insert in M the ones with w == ALWAYS --> currently not being used

    // Compute n
    list* n = compute_n(n1, n2);

    // Insert in m the 1 hop neighbors that are the only path to a 2 hop node in N
    for( list_item* it = n->head; it; it = it->next ) {
        unsigned char* n2_neigh_id = it->data;

        list* aux = list_init();

        void* iterator = NULL;
        N1_Tuple* current_n1_tuple = NULL;
        hash_table_item* hit = NULL;
        while( (hit = hash_table_iterator_next(n1, &iterator)) ) {
            current_n1_tuple = hit->value;

            N2_Tuple* n2_tuple = list_find_item(current_n1_tuple->ns, &equalN2Tuple, n2_neigh_id);
            if( n2_tuple != NULL ) {
                unsigned char* x = malloc(sizeof(uuid_t));
                uuid_copy(x, current_n1_tuple->id);
                list_add_item_to_tail(aux, x);
            }
        }

        if( aux->size == 1 ) {
            list_add_item_to_tail(m, list_remove_head(aux));
        }
        list_delete(aux);
    }

    //
    list* s = compute_not_covered(n1, m, n);
    unsigned int not_covered = s->size;
    while( not_covered > 0 ) {
        double max_w = 0.0;
        unsigned int max_r = 0;
        unsigned int max_d = 0;
        unsigned char* max_id = NULL;

        for( list_item* it = s->head; it; it = it->next ) {
            unsigned char* x = it->data;
            N1_Tuple* n1_tuple = hash_table_find_value(n1, x);

            if( n1_tuple->w > max_w || max_id == NULL ) {
                unsigned int r = compute_r(n1, m, n, n1_tuple);
                if( r > max_r || max_id == NULL ) {
                    unsigned int d = compute_d(n1, n, n1_tuple);
                    if( d > max_d || max_id == NULL ) {

                        // TODO: apply other tie breakers

                        max_w = n1_tuple->w;
                        max_r = r;
                        max_d = d;
                        max_id = n1_tuple->id;
                    }
                }
            }
        }

        assert( max_id != NULL );

        unsigned char* id = malloc(sizeof(uuid_t));
        uuid_copy(id, max_id);
        list_add_item_to_tail(m, id);

        list_delete(s);
        s = compute_not_covered(n1, m, n);
        not_covered = s->size;
    }
    list_delete(s);

    // Minimize M (optional)
    // TODO

    // Temp
    /*
    void* iterator = NULL;
    hash_table_item* hit = NULL;
    N1_Tuple* n1_tuple = NULL;
    while ( (hit = hash_table_iterator_next(n1, &iterator)) ) {
        n1_tuple = (N1_Tuple*)hit->value;

        unsigned char* id = malloc(sizeof(uuid_t));
        uuid_copy(id, n1_tuple->id);
        list_add_item_to_tail(m, id);
    }
*/

    return m;
}

void delete_n1_item(hash_table_item* hit, void* aux) {
    N1_Tuple* n1_tuple = (N1_Tuple*)hit->value;
    list_delete(n1_tuple->ns);
    free(n1_tuple);
}
