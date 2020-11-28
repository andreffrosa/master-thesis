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

#include "Yggdrasil/core/ygg_runtime.h"

#include "../bcast_header.h"

#include "data_structures/list.h"
#include "data_structures/double_list.h"

typedef struct _phase_stats phase_stats;

typedef struct _PendingMessage PendingMessage;

typedef struct _message_copy message_copy;

void setPhaseStats(phase_stats* ps, struct timespec* start_time, unsigned long duration, bool finished, bool retransmission_decision);

unsigned long getPhaseDuration(phase_stats* ps);

bool isPhaseFinished(phase_stats* ps);

bool getPhaseDecision(phase_stats* ps);

////////////////////////////////////////////////////////

message_copy* newMessageCopy(struct timespec* reception_time, bcast_header* header, void* context_header, unsigned int phase);

struct timespec* getCopyReceptionTime(message_copy* msg_copy);

bcast_header* getBcastHeader(message_copy* msg_copy);

void* getContextHeader(message_copy* msg_copy);

unsigned int getCopyPhase(message_copy* msg_copy);

////////////////////////////////////////////////////////////

PendingMessage* newPendingMessage(unsigned char* id, short proto_origin, void* payload, int length, int total_phases);

void deletePendingMessage(PendingMessage* p_msg);

unsigned char* getPendingMessageID(PendingMessage* p_msg);

unsigned int getCurrentPhase(PendingMessage* p_msg);

void incCurrentPhase(PendingMessage* p_msg);

void addMessageCopy(PendingMessage* p_msg, struct timespec* reception_time, bcast_header* header, void* context_header);

message_copy* getCopyFrom(PendingMessage* p_msg, uuid_t id);

bool receivedCopyFrom(PendingMessage* p_msg, uuid_t id);

YggMessage* getToDeliver(PendingMessage* p_msg);

bool isPendingMessageActive(PendingMessage* p_msg);

double_list* getCopies(PendingMessage* p_msg);

unsigned int getCurrentPhase(PendingMessage* p_msg);

phase_stats* getPhaseStats(PendingMessage* p_msg, unsigned int phase);

void splitDuration(PendingMessage* p_msg, struct timespec* current_time, unsigned long* elapsed, unsigned long* remaining);

void setCurrentPhaseDuration(PendingMessage* p_msg, unsigned long new_duration);

unsigned long getCurrentPhaseDuration(PendingMessage* p_msg);

void finishCurrentPhase(PendingMessage* p_msg);

void setCurrentPhaseStart(PendingMessage* p_msg, struct timespec* start_time);

void setMessageInactive(PendingMessage* p_msg);

void setPendingMessageDecision(PendingMessage* p_msg, bool decision);

void setCurrentPhaseStats(PendingMessage* p_msg, struct timespec* start_time, unsigned long duration, bool finished, bool retransmission_decision);

#endif /* _PENDING_MESSAGE_H_ */
