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

#include "pending_messages.h"

#include "utility/my_time.h"

typedef struct _PendingMessages {
	double_list* messages;
} PendingMessages;

// Comparator Function
bool equalPendingMessage(PendingMessage* a, uuid_t b) {
	return uuid_compare(getPendingMessageID(a), b) == 0;
}

PendingMessages* newPendingMessages() {
    PendingMessages* pending_msgs = malloc(sizeof(PendingMessages));
	pending_msgs->messages = double_list_init();
	return pending_msgs;
}

int deletePendingMessages(PendingMessages* pending_msgs) {

	int counter = 0;

	while(pending_msgs->messages->head != NULL) {
		PendingMessage* it  = double_list_remove_head(pending_msgs->messages);

		deletePendingMessage(it);

		free(it);

		counter++;
	}

	free(pending_msgs->messages);
	free(pending_msgs);

	return counter;
}

void pushPendingMessage(PendingMessages* p_msgs, PendingMessage* p_msg) {
	double_list_add_item_to_tail(p_msgs->messages, (void*) p_msg);
}

PendingMessage* popPendingMessage(PendingMessages* p_msgs, PendingMessage* p_msg) {
	return (PendingMessage*) double_list_remove(p_msgs->messages, (comparator_function) &equalPendingMessage, getPendingMessageID(p_msg));
}

PendingMessage* getPendingMessage(PendingMessages* p_msgs, uuid_t id) {
	return (PendingMessage*) double_list_find(p_msgs->messages, (comparator_function) &equalPendingMessage, id);
}

bool isInSeenMessages(PendingMessages* p_msgs, uuid_t id) {
	return (PendingMessage*) double_list_find_item(p_msgs->messages, (comparator_function) &equalPendingMessage, id) != NULL;
}


unsigned int runGarbageCollectorPM(PendingMessages* p_msgs, struct timespec* current_time, struct timespec* seen_expiration) {

	unsigned int counter = 0;

	double_list_item* current_item = p_msgs->messages->head;
	while(current_item != NULL) {
		PendingMessage* current_msg = (PendingMessage*) current_item->data;

		if(!isPendingMessageActive(current_msg)) {
			struct timespec* reception_time = getCopyReceptionTime((MessageCopy*)getCopies(current_msg)->head->data);

            struct timespec expiration = {0, 0};
			add_timespec(&expiration, reception_time, seen_expiration);

            // Message expired
			if( compare_timespec(&expiration, current_time) <= 0 ) {
				double_list_item* aux = current_item;
				current_item = current_item->next;

				PendingMessage* aux_msg = double_list_remove_item(p_msgs->messages, aux);

                deletePendingMessage(aux_msg);

				counter++;
			} else
				current_item = current_item->next;
		} else
			current_item = current_item->next;
	}

	return counter;
}
