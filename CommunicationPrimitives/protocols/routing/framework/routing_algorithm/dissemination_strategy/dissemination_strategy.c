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

#include "dissemination_strategy_private.h"

#include <assert.h>

DisseminationStrategy* newDisseminationStrategy(void* args, void* vars, ds_disseminate disseminate, ds_destroy destroy) {
    assert(disseminate);

    DisseminationStrategy* ds = malloc(sizeof(DisseminationStrategy));

    ds->state.args = args;
    ds->state.vars = vars;
    ds->disseminate = disseminate;
    ds->destroy = destroy;

    return ds;

}

void destroyDisseminationStrategy(DisseminationStrategy* ds) {
    if(ds) {
        if(ds->destroy) {
            ds->destroy(&ds->state);
        }
        free(ds);
    }
}

void DS_disseminate(DisseminationStrategy* ds, YggMessage* msg) {
    assert(ds);

    ds->disseminate(&ds->state, msg);
}
