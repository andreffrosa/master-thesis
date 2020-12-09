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
#include "pending_message.h"

#include "utility/my_time.h"

#include <assert.h>

typedef struct PhaseStats_ {
	struct timespec start_time;		       // Timestamp of the moment the phase started
    unsigned long duration;                // Duration of the phase
    bool finished;                         // Phase has finished
    bool retransmission_decision;          // Retransmission decison made on the phase. only valid if finished is true;
} PhaseStats;

typedef struct PendingMessage_ {
	uuid_t id;					   			// Message id
	YggMessage toDeliver;               	// Message to Deliver
	bool active;                   			// Flag that indicates that the current message is still active, and thus cannot be discarded

	double_list* copies;                  	// List of the headers of the received copies

	unsigned int current_phase;      		// Message's current phase

    PhaseStats* PhaseStats;
} PendingMessage;

typedef struct MessageCopy_ {
	struct timespec reception_time;		    // Timestamp of the moment the copy was received
	BroadcastHeader header;					// Broadcast Header
	void* context_header;			        // Retransmission Context
	unsigned int phase;						// Phase in which the copy was received
} MessageCopy;


bool equalMessageCopy(MessageCopy* a, uuid_t b) {
	return uuid_compare(a->header.sender_id, b) == 0;
}

void setPhaseStats(PhaseStats* ps, struct timespec* start_time, unsigned long duration, bool finished, bool retransmission_decision) {
    assert(ps != NULL);

    memcpy(&ps->start_time, start_time, sizeof(struct timespec));
    ps->duration = duration;
    ps->finished = finished;
    ps->retransmission_decision = retransmission_decision;
}

unsigned long getPhaseDuration(PhaseStats* ps) {
    assert(ps != NULL);
    return ps->duration;
}

bool isPhaseFinished(PhaseStats* ps) {
    assert(ps != NULL);
    return ps->finished;
}

bool getPhaseDecision(PhaseStats* ps) {
    assert(ps != NULL && ps->finished);
    return ps->retransmission_decision;
}

////////////////////////////////////////////////////////

MessageCopy* newMessageCopy(struct timespec* reception_time, BroadcastHeader* header, void* context_header, unsigned int phase) {
    MessageCopy* p_msg_copy = (MessageCopy*) malloc(sizeof(MessageCopy));

    memcpy(&p_msg_copy->reception_time, reception_time, sizeof(struct timespec));
	memcpy(&p_msg_copy->header, header, BROADCAST_HEADER_LENGTH);
	p_msg_copy->context_header = context_header;
	p_msg_copy->phase = phase;

    return p_msg_copy;
}

struct timespec* getCopyReceptionTime(MessageCopy* p_msg_copy) {
    assert(p_msg_copy);
    return &p_msg_copy->reception_time;
}

BroadcastHeader* getBcastHeader(MessageCopy* p_msg_copy) {
    assert(p_msg_copy);
    return &p_msg_copy->header;
}

void* getContextHeader(MessageCopy* p_msg_copy) {
    assert(p_msg_copy);
    return p_msg_copy->context_header;
}

unsigned int getCopyPhase(MessageCopy* p_msg_copy) {
    assert(p_msg_copy);
    return p_msg_copy->phase;
}

////////////////////////////////////////////////////////////

PendingMessage* newPendingMessage(unsigned char* id, short proto_origin, void* payload, int length, int total_phases) {
	PendingMessage* p_msg = malloc(sizeof(PendingMessage));

	uuid_copy(p_msg->id, id);

	YggMessage_initBcast(&(p_msg->toDeliver), proto_origin);
	YggMessage_addPayload(&(p_msg->toDeliver), payload, length);

	p_msg->copies = double_list_init();

	p_msg->active = true;
	p_msg->current_phase = 1;

    p_msg->PhaseStats = malloc(total_phases*sizeof(PhaseStats));
    memset(p_msg->PhaseStats, 0, total_phases*sizeof(PhaseStats));

	return p_msg;
}

void deletePendingMessage(PendingMessage* p_msg) {
	assert(p_msg != NULL);

    free(p_msg->PhaseStats);

	while(p_msg->copies->head != NULL) {
		MessageCopy* p_msg_copy  = double_list_remove_head(p_msg->copies);

		if(p_msg_copy->context_header != NULL)
			free(p_msg_copy->context_header);

		free(p_msg_copy);
	}

	free(p_msg->copies);
	free(p_msg);
}

unsigned char* getPendingMessageID(PendingMessage* p_msg) {
    assert(p_msg != NULL);
    return p_msg->id;
}

unsigned int getCurrentPhase(PendingMessage* p_msg) {
    assert(p_msg != NULL);
    return p_msg->current_phase;
}

void addMessageCopy(PendingMessage* p_msg, struct timespec* reception_time, BroadcastHeader* header, void* context_header) {
    assert(p_msg != NULL);
    assert(p_msg->copies != NULL);

    MessageCopy* p_msg_copy = newMessageCopy(reception_time, header, context_header, p_msg->current_phase);

    double_list_add_item_to_tail(p_msg->copies, p_msg_copy);
}

MessageCopy* getCopyFrom(PendingMessage* p_msg, uuid_t id) {
    assert(p_msg != NULL);
    assert(p_msg->copies != NULL);
	return (MessageCopy*) double_list_find(p_msg->copies, (comparator_function) &equalMessageCopy, id);
}

bool receivedCopyFrom(PendingMessage* p_msg, uuid_t id) {
    assert(p_msg != NULL);
    return getCopyFrom(p_msg, id) != NULL;
}

YggMessage* getToDeliver(PendingMessage* p_msg) {
    assert(p_msg != NULL);
    return &p_msg->toDeliver;
}

bool isPendingMessageActive(PendingMessage* p_msg) {
    assert(p_msg != NULL);
    return p_msg->active;
}

double_list* getCopies(PendingMessage* p_msg) {
    assert(p_msg != NULL);
    assert(p_msg->copies != NULL);

    return p_msg->copies;
}

PhaseStats* getPhaseStats(PendingMessage* p_msg, unsigned int phase) {
    assert(p_msg != NULL);
    assert(p_msg->PhaseStats);
    return &p_msg->PhaseStats[phase-1];
}

/////////////////////////////////////////////////////////////

void splitDuration(PendingMessage* p_msg, struct timespec* current_time, unsigned long* elapsed, unsigned long* remaining) {
    assert(p_msg != NULL);
    assert(p_msg->PhaseStats);

    PhaseStats* ps = &(p_msg->PhaseStats[p_msg->current_phase-1]);

    struct timespec t;
    subtract_timespec(&t, current_time, &ps->start_time);

    unsigned long elapsed_ = timespec_to_milli(&t);
    unsigned long remaining_ = elapsed_ >= ps->duration ? 0L : ps->duration - elapsed_;

    if(elapsed)
        *elapsed = elapsed_;

    if(remaining)
        *remaining = remaining_;
}

void setCurrentPhaseDuration(PendingMessage* p_msg, unsigned long new_duration) {
    assert(p_msg != NULL);
    assert(p_msg->PhaseStats);

    p_msg->PhaseStats[p_msg->current_phase-1].duration = new_duration;
}

unsigned long getCurrentPhaseDuration(PendingMessage* p_msg) {
    assert(p_msg != NULL);
    assert(p_msg->PhaseStats);

    return p_msg->PhaseStats[p_msg->current_phase-1].duration;
}

void finishCurrentPhase(PendingMessage* p_msg) {
    assert(p_msg != NULL);
    assert(p_msg->PhaseStats);

    p_msg->PhaseStats[p_msg->current_phase-1].finished = true;
}

void setCurrentPhaseStats(PendingMessage* p_msg, struct timespec* start_time, unsigned long duration, bool finished, bool retransmission_decision) {
    assert(p_msg != NULL);
    assert(p_msg->PhaseStats);

    setPhaseStats(&p_msg->PhaseStats[p_msg->current_phase-1], start_time, duration, finished, retransmission_decision);
}

void incCurrentPhase(PendingMessage* p_msg) {
    p_msg->current_phase++;
}

void setCurrentPhaseStart(PendingMessage* p_msg, struct timespec* start_time) {
    assert(p_msg != NULL);
    assert(p_msg->PhaseStats);

    PhaseStats* ps = &p_msg->PhaseStats[p_msg->current_phase-1];

    memcpy(&ps->start_time, start_time, sizeof(struct timespec));
    ps->finished = false;
    ps->retransmission_decision = false;
    ps->duration = 0L;
}

void setMessageInactive(PendingMessage* p_msg) {
    assert(p_msg != NULL);

    p_msg->active = false;
}

void setPendingMessageDecision(PendingMessage* p_msg, bool decision) {
    assert(p_msg != NULL);
    assert(p_msg->PhaseStats);
    
    p_msg->PhaseStats[p_msg->current_phase-1].retransmission_decision = decision;
}
