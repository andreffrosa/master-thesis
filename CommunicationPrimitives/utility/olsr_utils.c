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
#include "utility/my_math.h"

bool equalN2Tuple(void* a, void* b) {
    N2_Tuple* x = a, *y = b;
    return uuid_compare(x->id, y->id) == 0;
}

N1_Tuple* newN1Tuple(unsigned char* id, double d1, double w, list* ns, bool already_mpr) {
    N1_Tuple* x = malloc(sizeof(N1_Tuple));

    uuid_copy(x->id, id);
    x->d1 = d1;
    x->w = w;
    x->ns = ns;
    x->already_mpr = already_mpr;

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

/*
n is the set of nodes who are strict two hop neighbors or are also 1-hop neighbors yet there exists a better path through another 1-hop neighbor than the direct link.
*/

static list* compute_n(hash_table* n1, list* n2) {
    list* n = list_init();

    //printf("COMPUTING N\n");

    N1_Tuple* y_n1_tuple = NULL;
    unsigned char* y = NULL;
    for( list_item* it = n2->head; it; it = it->next ) {
        y = it->data;
        y_n1_tuple = hash_table_find_value(n1, y);

        // If strict two hop
        if( y_n1_tuple == NULL ) {
            unsigned char* aux = malloc(sizeof(uuid_t));
            uuid_copy(aux, y);
            list_add_item_to_tail(n, aux);


            /*
char id_str[UUID_STR_LEN+1] = {0};
            uuid_unparse(it->data, id_str);
            printf("%s is strict two hop\n", id_str);
*/


        }

        // Is also 1-hop neighbor
        else {
            double d1_y = y_n1_tuple->d1;

            void* iterator = NULL;
            N1_Tuple* x_n1_tuple = NULL;
            hash_table_item* hit = NULL;
            while( (hit = hash_table_iterator_next(n1, &iterator)) ) {
                x_n1_tuple = hit->value;

                if( uuid_compare(x_n1_tuple->id, y) != 0 ) { // x must be different than

                    N2_Tuple* n2_tuple = list_find_item(x_n1_tuple->ns, &equalN2Tuple, y);
                    if( n2_tuple != NULL ) {
                        // assert(uuid_compare(y, n2_tuple->id) == 0);

                        double d_x_y = n2_tuple->d2;

                        // Exists a better path to y through neighbor x
                        if( d_x_y > d1_y ) {
                            unsigned char* aux = malloc(sizeof(uuid_t));
                            uuid_copy(aux, y);
                            list_add_item_to_tail(n, aux);


                            /*
char id_str[UUID_STR_LEN+1] = {0};
                            uuid_unparse(y, id_str);
                            char id_str2[UUID_STR_LEN+1] = {0};
                            uuid_unparse(x_n1_tuple->id, id_str2);
                            printf("%s exists a better path through %s : my_d = %f its_d = %f %.15f\n", id_str, id_str2, d1_y, d_x_y, (d_x_y - d1_y));
*/


                        }
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

    if(list_find_item(m, &equalID, x->id) != NULL) {
        return 0.0;
    }

    unsigned int counter = 0;

    for( list_item* it = n->head; it; it = it->next ) {
        unsigned char* y = it->data;

        N2_Tuple* y_n2_tuple = list_find_item(x->ns, &equalN2Tuple, y);
        if(y_n2_tuple) { // if d(x,y) is defined
            double d_x_y = y_n2_tuple->d2;

            double max_value = 0.0;
            bool any = false;

            void* iterator = NULL;
            N1_Tuple* z = NULL;
            hash_table_item* hit = NULL;
            while( (hit = hash_table_iterator_next(n1, &iterator)) ) {
                z = hit->value;

                if( uuid_compare(z->id, x->id) != 0 ) {
                    if( list_find_item(m, &equalID, z->id) == NULL ) {
                        N2_Tuple* y_n2_tuple_2 = list_find_item(z->ns, &equalN2Tuple, y);
                        if(y_n2_tuple_2) {
                            double d_z_y = y_n2_tuple_2->d2;

                            max_value = dMax(max_value, d_z_y);
                            any = true;

                            /*

                            char id_str[UUID_STR_LEN+1] = {0};
                            uuid_unparse(z->id, id_str);
                            char id_str2[UUID_STR_LEN+1] = {0};
                            uuid_unparse(y_n2_tuple_2->id, id_str2);
                            printf("  z=%s is neigh of %s\n", id_str, id_str2);

                            */


                        }
                    }
                }
            }

            if( d_x_y == max_value && any ) {
                counter++;
            }
        }
    }

    return counter;
    //return r->size;
}

static unsigned int compute_d(hash_table* n1, list* n, N1_Tuple* x) {
    list* aux = list_init();
    unsigned int r = compute_r(n1, aux, n, x);
    list_delete(aux);
    return r;
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

    /*

    printf("Not covered:\n");
    for( list_item* it = s->head; it; it = it->next ) {
    char id_str[UUID_STR_LEN+1] = {0};
    uuid_unparse(it->data, id_str);
    printf("%s\n", id_str);
}

*/



return s;
}

list* compute_multipoint_relays(hash_table* n1, list* n2, list* initial) {

    // temp

    /*
    printf("N1:\n");
    void* iterator = NULL;
    hash_table_item* current_entry = NULL;
    while((current_entry = hash_table_iterator_next(n1, &iterator))) {
    char id_str[UUID_STR_LEN+1] = {0};
    uuid_unparse(current_entry->key, id_str);
    printf("%s\n", id_str);

    for( list_item* it = ((N1_Tuple*)current_entry->value)->ns->head; it; it = it->next ) {
    char id_str[UUID_STR_LEN+1] = {0};
    uuid_unparse(it->data, id_str);
    printf("    %s\n", id_str);
}
}
printf("N2:\n");
for( list_item* it = n2->head; it; it = it->next ) {
char id_str[UUID_STR_LEN+1] = {0};
uuid_unparse(it->data, id_str);
printf("%s\n", id_str);
}
*/




list* m = initial ? list_clone(initial, sizeof(uuid_t)) : list_init();

if(n2 && n2->size == 0) {
    return m;
}

// TODO: insert in M the ones with w == ALWAYS --> currently not being used

// Compute n = set of nodes who are strict two hop neighbors or there exists a better path through another 1-hop neighbor
list* n = compute_n(n1, n2);


/*
printf("N:\n");
for( list_item* it = n->head; it; it = it->next ) {
char id_str[UUID_STR_LEN+1] = {0};
uuid_unparse(it->data, id_str);
printf("%s\n", id_str);
}
*/



// Insert in m the 1 hop neighbors that are the only path to a node in N
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
        if( list_find_item(m, &equalID, aux->head->data) == NULL ) {
            list_add_item_to_tail(m, list_remove_head(aux));
        }
    }
    list_delete(aux);
}

/*
printf("M incomplete:\n");
for( list_item* it = m->head; it; it = it->next ) {
char id_str[UUID_STR_LEN+1] = {0};
uuid_unparse(it->data, id_str);
printf("%s\n", id_str);
}

*/



// While all the 1-hop neighbors are not covered
list* s = compute_not_covered(n1, m, n);
unsigned int not_covered = s->size;
while( not_covered > 0 ) {
    double max_w = 0.0;
    unsigned int max_r = 0;
    unsigned int max_d = 0;
    bool max_already_mpr = false;
    unsigned char* max_id = NULL;

    for( list_item* it = s->head; it; it = it->next ) {
        unsigned char* x = it->data;
        N1_Tuple* n1_tuple = hash_table_find_value(n1, x);

        bool select = false;

        unsigned int r = compute_r(n1, m, n, n1_tuple);
        unsigned int d = compute_d(n1, n, n1_tuple);

        if( max_id == NULL ) {
            select = true;
        } else {
            if( n1_tuple->w > max_w ) {
                select = true;
            } else if(n1_tuple->w == max_w) {
                if( r > max_r ) {
                    select = true;
                } else if( r == max_r ) {
                    if( d > max_d ) {
                        select = true;
                    } else if( d == max_d ) {

                        // New criteria
                        if( (n1_tuple->already_mpr && !max_already_mpr) ) {
                            select = true;
                        } /*else {
                            select = getRandomProb() <= 0.5; // If are tied, choose one randomly
                        }*/

                        // TODO: apply other tie breakers
                    }
                }
            }
        }

        if(select) {
            max_w = n1_tuple->w;
            max_r = r;
            max_d = d;
            max_already_mpr = n1_tuple->already_mpr;
            max_id = n1_tuple->id;
        }

    }

    assert( max_id != NULL );
    assert(list_find_item(m, &equalID, max_id) == NULL);

    unsigned char* id = malloc(sizeof(uuid_t));
    uuid_copy(id, max_id);
    list_add_item_to_tail(m, id);

    /*

    printf("M loop:\n");
    for( list_item* it = m->head; it; it = it->next ) {
    char id_str[UUID_STR_LEN+1] = {0};
    uuid_unparse(max_id, id_str);
    printf("%s\n", id_str);
    }

    */



    list_delete(s);
    s = compute_not_covered(n1, m, n);
    not_covered = s->size;
    }
    list_delete(s);


    /*

    printf("M final:\n");
    for( list_item* it = m->head; it; it = it->next ) {
    char id_str[UUID_STR_LEN+1] = {0};
    uuid_unparse(it->data, id_str);
    printf("%s\n", id_str);
    }

    */

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
