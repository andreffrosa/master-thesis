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

#include "link_quality_private.h"

#include <assert.h>

LinkQuality* newLinkQualityMetric(void* args, void* vars, lq_compute compute, lq_create_attrs create_attrs, lq_destroy_attrs destroy_attrs, lq_destroy destroy) {
    assert(compute);

    LinkQuality* lqm = malloc(sizeof(LinkQuality));

    lqm->state.args = args;
    lqm->state.vars = vars;
    lqm->compute = compute;
    lqm->create_attrs = create_attrs;
    lqm->destroy_attrs = destroy_attrs;
    lqm->destroy = destroy;

    return lqm;
}

void destroyLinkQualityMetric(LinkQuality* lqm/*, list* visited*/) {
    if(lqm) {
        if(lqm->destroy) {
            lqm->destroy(&lqm->state);
        }
        free(lqm);
    }
}

double LQ_compute(LinkQuality* lqm, void* lq_attrs, double previous_link_quality, unsigned int received, unsigned int lost, bool init, struct timespec* current_time) {
    assert(lqm);

    return lqm->compute(&lqm->state, lq_attrs, previous_link_quality, received, lost, init, current_time);
}

void* LQ_createAttrs(LinkQuality* lqm) {
    assert(lqm);

    if( lqm->create_attrs ) {
        return lqm->create_attrs(&lqm->state);
    } else {
        return NULL;
    }
}

void LQ_destroyAttrs(LinkQuality* lqm, void* lq_attrs) {
    assert(lqm);

    if( lq_attrs ) {
        if( lqm->destroy_attrs ) {
            lqm->destroy_attrs(&lqm->state, lq_attrs);
        } else {
            free(lq_attrs);
        }

    }
}
