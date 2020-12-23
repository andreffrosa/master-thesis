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

#ifndef _PENDING_MESSAGE_H_
#define _PENDING_MESSAGE_H_

#include "Yggdrasil.h"

#include "../broadcast_header.h"

#include "data_structures/list.h"
#include "data_structures/double_list.h"
#include "data_structures/hash_table.h"

#include "utility/byte.h"

typedef struct PhaseStats_ PhaseStats;

typedef struct PendingMessage_ PendingMessage;

typedef struct MessageCopy_ MessageCopy;

void setPhaseStats(PhaseStats* ps, struct timespec* start_time, unsigned long duration, bool finished, bool retransmission_decision);

unsigned long getPhaseDuration(PhaseStats* ps);

bool isPhaseFinished(PhaseStats* ps);

bool getPhaseDecision(PhaseStats* ps);

////////////////////////////////////////////////////////

MessageCopy* newMessageCopy(struct timespec* reception_time, BroadcastHeader* header, byte* context_header, unsigned int phase, hash_table* headers);

struct timespec* getCopyReceptionTime(MessageCopy* msg_copy);

BroadcastHeader* getBcastHeader(MessageCopy* msg_copy);

void* getContextHeader(MessageCopy* msg_copy);

unsigned int getCopyPhase(MessageCopy* msg_copy);

hash_table* getHeaders(MessageCopy* msg_copy);

////////////////////////////////////////////////////////////

PendingMessage* newPendingMessage(unsigned char* id, short proto_origin, void* payload, int length, int total_phases);

void deletePendingMessage(PendingMessage* p_msg);

unsigned char* getPendingMessageID(PendingMessage* p_msg);

unsigned int getCurrentPhase(PendingMessage* p_msg);

void incCurrentPhase(PendingMessage* p_msg);

void addMessageCopy(PendingMessage* p_msg, struct timespec* reception_time, BroadcastHeader* header, byte* context_header, hash_table* headers);

MessageCopy* getCopyFrom(PendingMessage* p_msg, uuid_t id);

bool receivedCopyFrom(PendingMessage* p_msg, uuid_t id);

YggMessage* getToDeliver(PendingMessage* p_msg);

bool isPendingMessageActive(PendingMessage* p_msg);

double_list* getCopies(PendingMessage* p_msg);

unsigned int getCurrentPhase(PendingMessage* p_msg);

PhaseStats* getPhaseStats(PendingMessage* p_msg, unsigned int phase);

void splitDuration(PendingMessage* p_msg, struct timespec* current_time, unsigned long* elapsed, unsigned long* remaining);

void setCurrentPhaseDuration(PendingMessage* p_msg, unsigned long new_duration);

unsigned long getCurrentPhaseDuration(PendingMessage* p_msg);

void finishCurrentPhase(PendingMessage* p_msg);

void setCurrentPhaseStart(PendingMessage* p_msg, struct timespec* start_time);

void setMessageInactive(PendingMessage* p_msg);

void setPendingMessageDecision(PendingMessage* p_msg, bool decision);

void setCurrentPhaseStats(PendingMessage* p_msg, struct timespec* start_time, unsigned long duration, bool finished, bool retransmission_decision);

#endif /* _PENDING_MESSAGE_H_ */
