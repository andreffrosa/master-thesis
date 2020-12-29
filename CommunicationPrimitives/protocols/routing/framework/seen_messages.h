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

#ifndef _ROUTING_FRAMEWORK_SEEN_MESSAGES_H_
#define _ROUTING_FRAMEWORK_SEEN_MESSAGES_H_

#include "Yggdrasil.h"

typedef struct _SeenMessages SeenMessages;

SeenMessages* newSeenMessages();

void destroySeenMessages(SeenMessages* m);

bool SeenMessagesContains(SeenMessages* m, unsigned char* id);

void SeenMessagesAdd(SeenMessages* m, unsigned char* id, struct timespec* current_time);

unsigned int SeenMessagesGC(SeenMessages* m, struct timespec* current_time, struct timespec* seen_expiration);

#endif /* _ROUTING_FRAMEWORK_SEEN_MESSAGES_H_ */
