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

#include "announce_period_private.h"

#include <assert.h>

void destroyAnnouncePeriod(AnnouncePeriod* ap) {
    if(ap) {
        free(ap);
    }
}

unsigned int AP_get(AnnouncePeriod* ap) {
    assert(ap);

    return ap->period;
}


AnnouncePeriod* StaticAnnouncePeriod(unsigned int p) {
    AnnouncePeriod* ap = malloc(sizeof(unsigned int));

    ap->period = p;

    return ap;
}
