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

#include "handlers.h"

#include "utility/my_math.h"
#include "utility/my_time.h"
#include "utility/my_misc.h"
#include "utility/my_string.h"

#include "debug.h"

#include "broadcast_algorithm/bcast_algorithms.h"

#include <limits.h>

#include <assert.h>

void init(broadcast_framework_state* state) {
    state->seen_msgs = newPendingMessages();

	// Garbage Collector
	genUUID(state->gc_id);
	struct timespec _gc_interval = {0, 0};
	milli_to_timespec(&_gc_interval, state->args->gc_interval_s*1000);
	SetPeriodicTimer(&_gc_interval, state->gc_id, BROADCAST_FRAMEWORK_PROTO_ID, -1);

	memset(&state->stats, 0, sizeof(broadcast_stats));

	getmyId(state->myID);

	clock_gettime(CLOCK_MONOTONIC, &state->current_time); // Update current time
}

void ComputeRetransmissionDelay(broadcast_framework_state* state, PendingMessage* p_msg, bool isCopy) {

    assert(isPendingMessageActive(p_msg));

    unsigned long elapsed, remaining;
    splitDuration(p_msg, &state->current_time, &elapsed, &remaining);

	unsigned long delay = BA_computeRetransmissionDelay(state->args->algorithms[getAlg(p_msg)], p_msg, remaining, isCopy, state->myID);

    setCurrentPhaseDuration(p_msg, elapsed + delay);

    struct timespec _delay = {0, 0};
    milli_to_timespec(&_delay, delay);
    if(timespec_is_zero(&_delay))
        _delay.tv_nsec = 1;

    if( !isCopy ) {
        //memcpy(&msg->timer_duration, &delay, sizeof(struct timespec));
        //timespec_add(&p_msg->timer_expiration_date, current_time, &delay);

        SetTimer(&_delay, getPendingMessageID(p_msg), BROADCAST_FRAMEWORK_PROTO_ID, TIMER_BROADCAST_MESSAGE_TIMEOUT);
    }
    else {
        if( delay < remaining ) {
            CancelTimer(getPendingMessageID(p_msg), BROADCAST_FRAMEWORK_PROTO_ID);
            SetTimer(&_delay, getPendingMessageID(p_msg), BROADCAST_FRAMEWORK_PROTO_ID, TIMER_BROADCAST_MESSAGE_TIMEOUT);
        }
    }

    #if DEBUG_INCLUDE_GT(BROADCAST_DEBUG_LEVEL, ADVANCED_DEBUG)
    {
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(getPendingMessageID(p_msg), id_str);
        char str[300];

        unsigned int current_phase = getCurrentPhase(p_msg);
        if(isCopy) {
            sprintf(str, "[%s] copy = %d phase = %d elapsed = %lu old_remaining = %lu new_remaining = %lu", id_str, getCopies(p_msg)->size, current_phase, elapsed, remaining, delay);
        } else {
            sprintf(str, "[%s] change to phase = %d previous_phase_duration = %lu delay = %lu", id_str, current_phase, current_phase == 1? 0:getPhaseDuration(getPhaseStats(p_msg, current_phase-1)), delay);
        }
        ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "DELAY", str);
    }
    #endif
}

void changePhase(broadcast_framework_state* state, PendingMessage* p_msg) {

    finishCurrentPhase(p_msg);

	if( getCurrentPhase(p_msg) < BA_getRetransmissionPhases(state->args->algorithms[getAlg(p_msg)]) ) {
        // Increment current current_phase
        incCurrentPhase(p_msg);

        setCurrentPhaseStart(p_msg, &state->current_time);

		// Compute Current Phase's Retransmission Delay
		ComputeRetransmissionDelay(state, p_msg, false);
	} else {
		// Message is no longer active
		setMessageInactive(p_msg);

        if(state->args->late_delivery) {
            // Deliver the message to the upper layer
            DeliverMessage(state, p_msg);
        }
	}
}

void DeliverMessage(broadcast_framework_state* state, PendingMessage* p_msg) {

    int add_result = 0;

    YggMessage* originalToDeliver = getToDeliver(p_msg);

    YggMessage toDeliver;
    YggMessage_initBcast(&toDeliver, originalToDeliver->Proto_id);

    // Insert src proto
    unsigned short src_proto = BROADCAST_FRAMEWORK_PROTO_ID;
    add_result = YggMessage_addPayload(&toDeliver, (char*)&src_proto, sizeof(src_proto));
    assert(add_result != FAILED);

    // Insert payload size
    unsigned short payload_size = originalToDeliver->dataLen;
    add_result = YggMessage_addPayload(&toDeliver, (char*)&payload_size, sizeof(payload_size));
    assert(add_result != FAILED);

    // Insert payload
    add_result = YggMessage_addPayload(&toDeliver, (char*)originalToDeliver->data, payload_size);
    assert(add_result != FAILED);

    double_list* copies = getCopies(p_msg);
    byte n_copies = copies->size;
    assert(copies && n_copies > 0);

    // Insert Broadcast Source Node's ID
    unsigned char* source_id = getBcastHeader(((MessageCopy*)copies->head->data))->source_id;
    add_result = YggMessage_addPayload(&toDeliver, (char*)source_id, sizeof(uuid_t));
    assert(add_result != FAILED);

    // Insert number of copies received
    add_result = YggMessage_addPayload(&toDeliver, (char*)&n_copies, sizeof(n_copies));
    assert(add_result != FAILED);

    for(double_list_item* dit = copies->head; dit; dit = dit->next) {
        MessageCopy* c = (MessageCopy*)dit->data;

        // Insert parent id
        unsigned char* parent_id = getBcastHeader(c)->sender_id;
        add_result = YggMessage_addPayload(&toDeliver, (char*)parent_id, sizeof(uuid_t));
        assert(add_result != FAILED);

        // Insert timestamp
        struct timespec* t = getCopyReceptionTime(c);
        add_result = YggMessage_addPayload(&toDeliver, (char*)t, sizeof(struct timespec));
        assert(add_result != FAILED);

        // Insert context length
        unsigned short context_length = getBcastHeader(c)->context_length;
        add_result = YggMessage_addPayload(&toDeliver, (char*)&context_length, sizeof(context_length));
        assert(add_result != FAILED);

        // Insert Context
        void* context = getContextHeader(c);
        assert( (context_length == 0) || (context != NULL) );
        add_result = YggMessage_addPayload(&toDeliver, (char*)context, context_length);
        assert(add_result != FAILED);
    }

    #if DEBUG_INCLUDE_GT(BROADCAST_DEBUG_LEVEL, SIMPLE_DEBUG)
    {
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(getPendingMessageID(p_msg), id_str);
        char str[UUID_STR_LEN+4];
        sprintf(str, "[%s]", id_str);
        ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "DELIVER", str);
    }
    #endif

    //YggMessage* toDeliver = getToDeliver(p_msg);
    deliver(&toDeliver);

	state->stats.messages_delivered++;
}

void RetransmitMessage(broadcast_framework_state* state, PendingMessage* p_msg, unsigned short ttl) {
    YggMessage m;

    serializeMessage(state, &m, p_msg, ttl);

	pushMessageType(&m, MSG_BROADCAST_MESSAGE);

	dispatch(&m);

	state->stats.messages_transmitted++;
}

void uponBroadcastRequest(broadcast_framework_state* state, YggRequest* req) {
    state->stats.messages_bcasted++;

    unsigned short ttl = 0;
    void* ptr = YggRequest_readPayload(req, NULL, &ttl, sizeof(ttl));

    unsigned int alg = 0;
    ptr = YggRequest_readPayload(req, ptr, &alg, sizeof(alg));
    assert(alg < state->args->algorithms_length);

    unsigned short size = req->length - sizeof(ttl);
    byte data[size];
    ptr = YggRequest_readPayload(req, ptr, (char*)data, size);

	// Generate a unique message id
	uuid_t msg_id;
	genUUID(msg_id);

	// Create a new message item
    PendingMessage* p_msg = newPendingMessage(msg_id, req->proto_origin, alg, data, size, BA_getRetransmissionPhases(state->args->algorithms[alg]));
    setCurrentPhaseStats(p_msg, &state->current_time, 0L, true, true);

    // Add the first copy
    BroadcastHeader header = {0};
    uuid_copy(header.source_id, state->myID);
    uuid_copy(header.sender_id, state->myID);
    uuid_copy(header.msg_id, msg_id);
    header.dest_proto = req->proto_origin;
    header.ttl = ttl;

    // The context is empty by default on requests
    header.context_length = 0;
    byte* context_header = NULL;
    //serializeHeader(state, p_msg, &header, &context_header, ttl);

    hash_table* headers = hash_table_init((hashing_function)&string_hash, (comparator_function)&equal_str);

    addMessageCopy(p_msg, &state->current_time, &header, context_header, headers);

    // Insert on seen_msgs
    pushPendingMessage(state->seen_msgs, p_msg);

    #if DEBUG_INCLUDE_GT(BROADCAST_DEBUG_LEVEL, SIMPLE_DEBUG)
    {
        YggMessage* ym = getToDeliver(p_msg);
        char content[ym->dataLen+1];
        memcpy(content, ym->data, ym->dataLen);
        content[ym->dataLen] = '\0';

        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(getPendingMessageID(p_msg), id_str);

        char ttl_str[10];
        if(ttl == USHRT_MAX) {
            sprintf(ttl_str, "infinite");
        } else {
            sprintf(ttl_str, "%d", ttl);
        }

        char str[ym->dataLen+UUID_STR_LEN+100];
        sprintf(str, "[%s] ttl=%s : %s", id_str, ttl_str, content);

        ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "BROADCAST REQ", str);
    }
    #endif

    if(!state->args->late_delivery) {
        // Deliver the message to the upper layer
        DeliverMessage(state, p_msg);
    }

    if(ttl > 0) {
        // Retransmit Message
    	RetransmitMessage(state, p_msg, ttl);
    }

	// Check if there are more phases
	changePhase(state, p_msg);
}

void uponNewMessage(broadcast_framework_state* state, YggMessage* msg) {

	state->stats.messages_received++;

	// Remove Framework's header of the message
	BroadcastHeader header;
	YggMessage toDeliver;
	byte* context_header;
	deserializeMessage(msg, &header, &context_header, &toDeliver);

    assert(header.ttl > 0);

    hash_table* headers = BA_parseHeader(state->args->algorithms[header.alg], header.context_length, context_header, state->myID);

    PendingMessage* p_msg = getPendingMessage(state->seen_msgs, header.msg_id);

	// If New Msg
	if( p_msg == NULL ){

		// Insert on seen_msgs
        p_msg = newPendingMessage(header.msg_id, header.dest_proto, header.alg, toDeliver.data, toDeliver.dataLen, BA_getRetransmissionPhases(state->args->algorithms[header.alg]));
        setCurrentPhaseStart(p_msg, &state->current_time);

        addMessageCopy(p_msg, &state->current_time, &header, context_header, headers);

        pushPendingMessage(state->seen_msgs, p_msg);

        if(!state->args->late_delivery) {
            // Deliver the message to the upper layer
            DeliverMessage(state, p_msg);
        }

        BA_processCopy(state->args->algorithms[getAlg(p_msg)], p_msg, state->myID);

		// Compute Current Phase's Retransmission Delay
		ComputeRetransmissionDelay(state, p_msg, false);

	} else {  // Duplicate message
        addMessageCopy(p_msg, &state->current_time, &header, context_header, headers);

        BA_processCopy(state->args->algorithms[getAlg(p_msg)], p_msg, state->myID);

        if( isPendingMessageActive(p_msg) ){
            // Compute Retransmission Delay
    		ComputeRetransmissionDelay(state, p_msg, true);
        }
	}

    #if DEBUG_INCLUDE_GT(BROADCAST_DEBUG_LEVEL, ADVANCED_DEBUG)
    {
        char id_str[UUID_STR_LEN+1];
        id_str[UUID_STR_LEN] = '\0';
        uuid_unparse(getPendingMessageID(p_msg), id_str);

        char parent_str[UUID_STR_LEN+1];
        parent_str[UUID_STR_LEN] = '\0';
        uuid_unparse(getBcastHeader((MessageCopy*)(getCopies(p_msg)->head->data))->sender_id, parent_str);
        char str[150];

        char ttl_str[10];
        if(header.ttl == USHRT_MAX) {
            sprintf(ttl_str, "infinite");
        } else {
            sprintf(ttl_str, "%d", header.ttl);
        }

        if(isPendingMessageActive(p_msg)) {
            sprintf(str, "[%s] %d copy on phase %d from %s with ttl=%s", id_str, getCopies(p_msg)->size, getCurrentPhase(p_msg), parent_str, ttl_str);
        } else {
            /*
            unsigned int current_phase = getCurrentPhase(p_msg);
            unsigned long total_duration = 0L;
            for(int i = 0; i < current_phase; i++) {
            total_duration += getPhaseDuration(getPhaseStats(p_msg, current_phase));
            }
            */
            sprintf(str, "[%s] %d copy on inactive from %s with ttl=%s", id_str, getCopies(p_msg)->size, parent_str, ttl_str);
        }

        ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "RECEIVED MESSAGE", str);
    }
    #endif
}

void uponTimeout(broadcast_framework_state* state, YggTimer* timer) {

    PendingMessage* p_msg = getPendingMessage(state->seen_msgs, timer->id);

	assert(p_msg != NULL);
	if( p_msg != NULL ) {
        unsigned long remaining;
        splitDuration(p_msg, &state->current_time, NULL, &remaining);

        if( remaining > 0 ) {
            // Phase not ended yet
            struct timespec _remaining = {0, 0};
        	milli_to_timespec(&_remaining, remaining);

        	SetTimer(&_remaining, getPendingMessageID(p_msg), BROADCAST_FRAMEWORK_PROTO_ID, TIMER_BROADCAST_MESSAGE_TIMEOUT);
        } else {
            // Find the maximum TTL received
            unsigned short max_ttl = 0;
            for(double_list_item* it = getCopies(p_msg)->head; it; it = it->next) {
                MessageCopy* msg_copy = (MessageCopy*)(it->data);
                max_ttl = iMax(max_ttl, getBcastHeader(msg_copy)->ttl);
            }

            // Decrement TTL
            if(max_ttl != USHRT_MAX) { // Different than infinity
                max_ttl--;
            }

            bool retransmit = false;
            if(max_ttl > 0) {
                // Execute Policy
        		retransmit = BA_evalRetransmissionPolicy(state->args->algorithms[getAlg(p_msg)], p_msg, state->myID);
            }/* else {
                retransmit = false;
            }*/

            setPendingMessageDecision(p_msg, retransmit);

    		if(retransmit) {
                // Retransmit Message
                RetransmitMessage(state, p_msg, max_ttl);
            } else {
                state->stats.messages_not_transmitted++;
            }

            #if DEBUG_INCLUDE_GT(BROADCAST_DEBUG_LEVEL, SIMPLE_DEBUG)
            {
                char id_str[UUID_STR_LEN+1];
                id_str[UUID_STR_LEN] = '\0';
                uuid_unparse(getPendingMessageID(p_msg), id_str);

                char ttl_str[10];
                if(max_ttl == USHRT_MAX) {
                    sprintf(ttl_str, "infinite");
                } else {
                    sprintf(ttl_str, "%d", max_ttl);
                }

                char str [100];
                sprintf(str, "[%s] Decision %s on phase %d max_ttl=%s", id_str, retransmit?"true":"false", getCurrentPhase(p_msg), ttl_str);
                ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "POLICY", str);
            }
            #endif

    		// Check if there are more phases
    		changePhase(state, p_msg);

    		/*if( compareTimespec(&current_time, &p_msg->expiration_date) >= 0 ) {
    			popFromPending(&state->pending_msgs, p_msg );

    			int retransmit = state->my_args->algorithm->retransmission->timeout(state->my_args->algorithm->retransmission->global_state, state->my_args->algorithm->retransmission->args, p_msg->phase, p_msg->retrasnmission_attr);
    			onEndOfPhase(state, p_msg, &current_time, retransmit);
    		} else {
    			SetTimer(&p_msg->expiration_date, timer->id, BROADCAST_FRAMEWORK_PROTO_ID);
    		}*/
        }
	} else {
		ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "TIMEOUT", "Received wierd timer!");
	}
}

void serializeHeader(broadcast_framework_state* state, PendingMessage* p_msg, BroadcastHeader* header, byte** context_header, unsigned short ttl) {
    uuid_copy(header->msg_id, getPendingMessageID(p_msg));
	getmyId(header->sender_id);
	header->dest_proto = getToDeliver(p_msg)->Proto_id;

    assert(getCopies(p_msg)->head != NULL);
    MessageCopy* first = (MessageCopy*)getCopies(p_msg)->head->data;
    assert(first != NULL);
    unsigned char* source_id = getBcastHeader(first)->source_id;
    uuid_copy(header->source_id, source_id);

	header->context_length = BA_createHeader(state->args->algorithms[getAlg(p_msg)], p_msg, context_header, state->myID, &state->current_time);

    header->ttl = ttl;

    header->alg = getAlg(p_msg);
}

void serializeMessage(broadcast_framework_state* state, YggMessage* m, PendingMessage* p_msg, unsigned short ttl) {

	assert(state != NULL);
	assert(m != NULL);
	assert(p_msg != NULL);

    YggMessage* toDeliver = getToDeliver(p_msg);

    YggMessage_initBcast(m, BROADCAST_FRAMEWORK_PROTO_ID);

	// Initialize Broadcast Message Header
	BroadcastHeader header;
    byte* context_header = NULL;
    serializeHeader(state, p_msg, &header, &context_header, ttl);

    int msg_size = BROADCAST_HEADER_LENGTH + header.context_length + toDeliver->dataLen;
    assert( msg_size <= YGG_MESSAGE_PAYLOAD );

	// Insert Header
	YggMessage_addPayload(m, (char*) &header, BROADCAST_HEADER_LENGTH);

	// Insert Context Header
	if( header.context_length > 0)
		YggMessage_addPayload(m, (char*) context_header, header.context_length);

	// Insert Payload
	YggMessage_addPayload(m, (char*) toDeliver->data, toDeliver->dataLen);

	// Free any allocated memory
	if(context_header != NULL )
		free(context_header);
}

void deserializeMessage(YggMessage* m, BroadcastHeader* header, byte** context_header, YggMessage* toDeliver) {

	void* ptr = NULL;
	ptr = YggMessage_readPayload(m, ptr, header, BROADCAST_HEADER_LENGTH);

	if( header->context_length > 0 ) {
		*context_header = malloc(header->context_length);
		ptr = YggMessage_readPayload(m, ptr, *context_header, header->context_length);
	} else {
		*context_header = NULL;
	}

	unsigned short payload_size = m->dataLen - (BROADCAST_HEADER_LENGTH + header->context_length);

	YggMessage_initBcast(toDeliver, header->dest_proto);
	memcpy(toDeliver->srcAddr.data, m->srcAddr.data, WLAN_ADDR_LEN);
	toDeliver->dataLen = payload_size;
	ptr = YggMessage_readPayload(m, ptr, toDeliver->data, payload_size);
}

void runGarbageCollector(broadcast_framework_state* state) {

    struct timespec seen_expiration = {0, 0};
    milli_to_timespec(&seen_expiration, state->args->seen_expiration_ms);

    #if DEBUG_INCLUDE_GT(BROADCAST_DEBUG_LEVEL, ADVANCED_DEBUG)
        unsigned int counter = runGarbageCollectorPM(state->seen_msgs, &state->current_time, &seen_expiration);

        char s[100];
        sprintf(s, "deleted %u messages.", counter);
        ygg_log(BROADCAST_FRAMEWORK_PROTO_NAME, "GC", s);
    #else
        runGarbageCollectorPM(state->seen_msgs, &state->current_time, &seen_expiration);
    #endif
}

void uponStatsRequest(broadcast_framework_state* state, YggRequest* req) {
	unsigned short dest = req->proto_origin;
	YggRequest_init(req, BROADCAST_FRAMEWORK_PROTO_ID, dest, REPLY, REQ_BROADCAST_FRAMEWORK_STATS);

	YggRequest_addPayload(req, (void*) &state->stats, sizeof(broadcast_stats));

	deliverReply(req);

	YggRequest_freePayload(req);
}
