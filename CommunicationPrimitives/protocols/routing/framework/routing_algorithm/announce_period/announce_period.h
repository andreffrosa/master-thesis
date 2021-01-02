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

#ifndef _ANNOUNCE_PERIOD_H_
#define _ANNOUNCE_PERIOD_H_

#include "../common.h"

typedef struct AnnouncePeriod_ AnnouncePeriod;

void destroyAnnouncePeriod(AnnouncePeriod* ap);

unsigned int AP_get(AnnouncePeriod* ap);

AnnouncePeriod* StaticAnnouncePeriod(unsigned int p);

#endif /*_ANNOUNCE_PERIOD_H_*/
