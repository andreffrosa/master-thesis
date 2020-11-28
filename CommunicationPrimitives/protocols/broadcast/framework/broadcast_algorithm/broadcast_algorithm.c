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

#include <stdlib.h>
#include <assert.h>

#include "broadcast_algorithm_private.h"

BroadcastAlgorithm* newBroadcastAlgorithm(RetransmissionContext* r_context, RetransmissionDelay* r_delay, RetransmissionPolicy* r_policy, unsigned int n_phases) {
	BroadcastAlgorithm* alg = (BroadcastAlgorithm*)malloc(sizeof(BroadcastAlgorithm));

	alg->r_context = r_context;
	alg->r_delay = r_delay;
	alg->r_policy = r_policy;
	alg->n_phases = n_phases;

	return alg;
}

void destroyBroadcastAlgorithm(BroadcastAlgorithm* alg) {
    if(alg != NULL) {
        destroyRetransmissionDelay(alg->r_delay, NULL);
        destroyRetransmissionPolicy(alg->r_policy, NULL);
        destroyRetransmissionContext(alg->r_context, NULL);
        free(alg);
    }
}

RetransmissionContext* getRetransmissionContext(BroadcastAlgorithm* alg) {
    assert(alg != NULL);
    return alg->r_context;
}

RetransmissionContext* setRetransmissionContext(BroadcastAlgorithm* alg, RetransmissionContext* new_context) {
    assert(alg != NULL);
    RetransmissionContext* old_context = alg->r_context;
    alg->r_context = new_context;
    return old_context;
}

RetransmissionDelay* getRetransmissionDelay(BroadcastAlgorithm* alg) {
    assert(alg != NULL);
    return alg->r_delay;
}

RetransmissionDelay* setRetransmissionDelay(BroadcastAlgorithm* alg, RetransmissionDelay* new_delay) {
    assert(alg != NULL);
    RetransmissionDelay* old_delay = alg->r_delay;
    alg->r_delay = new_delay;
    return old_delay;
}

RetransmissionPolicy* getRetransmissionPolicy(BroadcastAlgorithm* alg) {
    assert(alg != NULL);
    return alg->r_policy;
}

RetransmissionPolicy* setRetransmissionPolicy(BroadcastAlgorithm* alg, RetransmissionPolicy* new_policy) {
    assert(alg != NULL);
    RetransmissionPolicy* old_policy = alg->r_policy;
    alg->r_policy = new_policy;
    return old_policy;
}

unsigned int getRetransmissionPhases(BroadcastAlgorithm* alg) {
    assert(alg != NULL);
    return alg->n_phases;
}

void setRetransmissionPhases(BroadcastAlgorithm* alg, unsigned int phases) {
    assert(alg != NULL);
    alg->n_phases = phases;
}

unsigned long triggerRetransmissionDelay(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID) {
    assert(algorithm != NULL && p_msg != NULL);
    RetransmissionDelay* rd = algorithm->r_delay;
    RetransmissionContext* rc = algorithm->r_context;
	return rd->r_delay(&rd->delay_state, p_msg, remaining, isCopy, rc, myID);
}

bool triggerRetransmissionPolicy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID) {
    assert(algorithm != NULL);
    RetransmissionPolicy* rp = algorithm->r_policy;
    RetransmissionContext* rc = algorithm->r_context;
	return rp->r_policy(&rp->policy_state, p_msg, rc, myID);
}

void triggerRetransmissionContextInit(BroadcastAlgorithm* algorithm, proto_def* protocol_definition, unsigned char* myID) {
    assert(algorithm != NULL);
    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
    void** this = malloc(sizeof(void*));
    *this = rc;
    list_add_item_to_tail(visited, this);

	rc->init(&rc->context_state, protocol_definition, myID, visited);

    list_delete(visited);
}

void triggerRetransmissionContextEvent(BroadcastAlgorithm* algorithm, queue_t_elem* event, unsigned char* myID) {
    assert(algorithm != NULL);
    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
    void** this = malloc(sizeof(void*));
    *this = rc;
    list_add_item_to_tail(visited, this);

	rc->process_event(&rc->context_state, event, rc, myID, visited);

    list_delete(visited);
}

unsigned int triggerRetransmissionContextHeader(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, void** context_header, unsigned char* myID) {
    assert(algorithm != NULL && p_msg != NULL);
    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
    void** this = malloc(sizeof(void*));
    *this = rc;
    list_add_item_to_tail(visited, this);

	unsigned int result = rc->create_header(&rc->context_state, p_msg, context_header, rc, myID, visited);

    list_delete(visited);

    return result;
}

void triggerRetransmissionContextCopy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID) {
    assert(algorithm != NULL && p_msg != NULL);
    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
    void** this = malloc(sizeof(void*));
    *this = rc;
    list_add_item_to_tail(visited, this);

    if(rc->copy_handler != NULL)
        rc->copy_handler(&rc->context_state, p_msg, rc, myID, visited);

    list_delete(visited);
}
