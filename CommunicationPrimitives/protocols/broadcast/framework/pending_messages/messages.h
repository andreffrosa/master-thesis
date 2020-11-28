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

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#include "Yggdrasil/core/ygg_runtime.h"

#include "bcast_header.h"

#include "data_structures/list.h"
#include "data_structures/double_list.h"

typedef struct _message_list {
	double_list* messages;
} message_list;

typedef struct _phase_stats {
	struct timespec start_time;		       // Timestamp of the moment the phase started
    unsigned long duration;                // Duration of the phase
    bool finished;                         // Phase has finished
    bool retransmission_decision;          // Retransmission decison made on the phase. only valid if finished is true;
} phase_stats;

typedef struct _message_item {
	uuid_t id;					   			// Message id
	YggMessage toDeliver;               	// Message to Deliver
	bool active;                   			// Flag that indicates that the current message is still active, and thus cannot be discarded

	double_list* copies;                  	// List of the headers of the received copies

	int current_phase;      			    // Message's current phase
	//bool* retransmissions;     	   		// Retransmissions of the message on each phase

	//unsigned long timer_duration;			// Amount of time setup for the current timer
	//struct timespec timer_expiration_date;// Timestamp of the moment when the timer for the current phase will expire

    phase_stats* phase_stats;
} message_item;

typedef struct _message_copy {
	struct timespec reception_time;		    // Timestamp of the moment the copy was received
	bcast_header header;					// Broadcast Header
	void* context_header;			        // Retransmission Context
	unsigned int phase;						// Phase in which the copy was received

} message_copy;

// Functions

message_list* initMessagesList(message_list* m_list);

int deleteMessagesList(message_list* m_list);

message_item* newMessageItem(uuid_t id, short proto_origin, void* payload, int length, int phase, int total_phases, const struct timespec* reception_time, bcast_header* header, void* r_context, unsigned long timer_duration, struct timespec* current_time);

void deleteMessageItem(message_item* it);

void pushMessageItem(message_list* m_list, message_item* it);

message_item* popMessageItem(message_list* m_list, message_item* it);

message_item* getMessageItem(message_list* m_list, uuid_t id);

bool isInSeenMessages(message_list* m_list, uuid_t id);

void addCopy(message_item* msg_item, const struct timespec* reception_time, bcast_header* header, void* retransmission_context);

bool receivedCopyFrom(double_list* copies, uuid_t id);

message_copy* getCopyFrom(double_list* copies, uuid_t id);

void setPhaseStats(phase_stats* ps, struct timespec* start_time, unsigned long duration, bool finished, bool retransmission_decision);

void splitDuration(message_item* msg_item, struct timespec* current_time, unsigned long* elapsed, unsigned long* remaining);

//void validate_pending(pending_list* m_list);

#endif /* _MESSAGES_H_ */
