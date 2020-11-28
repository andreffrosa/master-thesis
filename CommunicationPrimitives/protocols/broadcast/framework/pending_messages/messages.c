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
 * (C) 2019
 *********************************************************/

#include "messages.h"

#include "utility/my_time.h"

#include <assert.h>

// Comparator Function
bool _message_item_equal(message_item* a, uuid_t b) {
	return uuid_compare(a->id, b) == 0;
}

bool _message_copy_equal(message_copy* a, uuid_t b) {
	return uuid_compare(a->header.sender_id, b) == 0;
}

message_list* initMessagesList(message_list* m_list) {
	m_list->messages = double_list_init();
	return m_list;
}

int deleteMessagesList(message_list* m_list) {

	int counter = 0;

	while(m_list->messages->head != NULL) {
		message_item* it  = double_list_remove_head(m_list->messages);

		deleteMessageItem(it);

		free(it);

		counter++;
	}

	free(m_list->messages);
	free(m_list);

	return counter;
}

message_item* newMessageItem(uuid_t id, short proto_origin, void* payload, int length, int phase, int total_phases, const struct timespec* reception_time, bcast_header* header, void* r_context, unsigned long timer_duration, struct timespec* current_time) {
	message_item* msg_item = malloc(sizeof(message_item));

	uuid_copy(msg_item->id, id);

	YggMessage_initBcast(&(msg_item->toDeliver), proto_origin);
	YggMessage_addPayload(&(msg_item->toDeliver), payload, length);

	msg_item->copies = double_list_init();

	if(header!= NULL)
		addCopy(msg_item, reception_time, header, r_context);

	msg_item->active = true;
	msg_item->current_phase = phase;

    msg_item->phase_stats = malloc(total_phases*sizeof(phase_stats));
    memset(msg_item->phase_stats, 0, total_phases*sizeof(phase_stats));
    memcpy(&msg_item->phase_stats[0].start_time, current_time, sizeof(struct timespec));
    msg_item->phase_stats[0].duration = timer_duration;
    msg_item->phase_stats[0].finished = false;
    msg_item->phase_stats[0].retransmission_decision = false;
    /*
    size_t len = (total_phases+1)*sizeof(bool);
	msg_item->retransmissions = malloc(len);
    memset(msg_item->retransmissions, false, len);

	msg_item->timer_duration = timer_duration;
    */

	//	memcpy(&it->timer_expiration_date, timer_expiration_date, sizeof(struct timespec));

	//	memcpy(&it->timer_duration, timer_duration, sizeof(struct timespec));

	return msg_item;
}

void deleteMessageItem(message_item* it) {
	assert(it != NULL);

    free(it->phase_stats);

	while(it->copies->head != NULL) {
		message_copy* msg_copy  = double_list_remove_head(it->copies);

		if(msg_copy->context_header != NULL)
			free(msg_copy->context_header);

		free(msg_copy);
	}

	free(it->copies);
	free(it);
}

void pushMessageItem(message_list* m_list, message_item* msg_item) {
	double_list_add_item_to_tail(m_list->messages, (void*) msg_item);
}

message_item* popMessageItem(message_list* m_list, message_item* it) {
	return (message_item*) double_list_remove(m_list->messages, (comparator_function) &_message_item_equal, it->id);
}

message_item* getMessageItem(message_list* m_list, uuid_t id) {
	return (message_item*) double_list_find(m_list->messages, (comparator_function) &_message_item_equal, id);
}

bool isInSeenMessages(message_list* m_list, uuid_t id) {
	return (message_item*) double_list_find_item(m_list->messages, (comparator_function) &_message_item_equal, id) != NULL;
}

void addCopy(message_item* msg_item, const struct timespec* reception_time, bcast_header* header, void* context_header) {

	message_copy* msg_copy = (message_copy*) malloc(sizeof(message_copy));

	memcpy(&msg_copy->reception_time, reception_time, sizeof(struct timespec));
	memcpy(&msg_copy->header, header, BCAST_HEADER_LENGTH);
	msg_copy->context_header = context_header;
	msg_copy->phase = msg_item->current_phase;

	double_list_add_item_to_tail(msg_item->copies, msg_copy);
}

message_copy* getCopyFrom(double_list* copies, uuid_t id) {
	return (message_copy*) double_list_find(copies, (comparator_function) &_message_copy_equal, id);
}

bool receivedCopyFrom(double_list* copies, uuid_t id) {
	return getCopyFrom(copies, id) != NULL;
}

void setPhaseStats(phase_stats* ps, struct timespec* start_time, unsigned long duration, bool finished, bool retransmission_decision) {
    memcpy(&ps->start_time, start_time, sizeof(struct timespec));
    ps->duration = duration;
    ps->finished = finished;
    ps->retransmission_decision = retransmission_decision;
}

void splitDuration(message_item* msg_item, struct timespec* current_time, unsigned long* elapsed, unsigned long* remaining) {
    phase_stats* ps = &(msg_item->phase_stats[msg_item->current_phase-1]);

    struct timespec t;
    subtract_timespec(&t, current_time, &ps->start_time);

    unsigned long elapsed_ = timespec_to_milli(&t);
    unsigned long remaining_ = ps->duration - elapsed_;

    if(elapsed)
        *elapsed = elapsed_;

    if(remaining)
        *remaining = remaining_;
}
