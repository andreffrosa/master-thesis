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

#include "link_admission_private.h"

#include <assert.h>

LinkAdmission* newLinkAdmissionPolicy(void* args, void* vars, la_eval eval, la_destroy destroy) {
    assert(eval);

    LinkAdmission* lap = malloc(sizeof(LinkAdmission));

    lap->state.args = args;
    lap->state.vars = vars;
    lap->eval = eval;
    //lap->create_attrs = create_attrs;
    //lap->destroy_attrs = destroy_attrs;
    lap->destroy = destroy;

    return lap;
}

void destroyLinkAdmissionPolicy(LinkAdmission* lap) {
    if(lap) {
        if(lap->destroy) {
            lap->destroy(&lap->state);
        }
        free(lap);
    }
}

bool LA_eval(LinkAdmission* lap, NeighborEntry* neigh, struct timespec* current_time) {
    assert(lap);

    return lap->eval(&lap->state, neigh, current_time);
}

/*
void* LA_createAttrs(LinkAdmission* lap) {
    assert(lap);

    if( lap->create_attrs ) {
        return lap->create_attrs(&lap->state);
    } else {
        return NULL;
    }
}

void LA_destroyAttrs(LinkAdmission* lap, void* lq_attrs) {
    assert(lap);

    if( lq_attrs ) {
        if( lap->destroy_attrs ) {
            lap->destroy_attrs(&lap->state, lq_attrs);
        } else {
            free(lq_attrs);
        }

    }
}
*/
