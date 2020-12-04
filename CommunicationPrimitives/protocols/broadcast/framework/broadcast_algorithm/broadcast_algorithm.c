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
        list* visited = list_init();
        destroyRetransmissionDelay(alg->r_delay, visited);
        list_delete(visited);

        visited = list_init();
        destroyRetransmissionPolicy(alg->r_policy, visited);
        list_delete(visited);

        visited = list_init();
        destroyRetransmissionContext(alg->r_context, visited);
        list_delete(visited);

        free(alg);
    }
}

RetransmissionContext* BA_getRetransmissionContext(BroadcastAlgorithm* alg) {
    assert(alg != NULL);

    return alg->r_context;
}

void BA_setRetransmissionContext(BroadcastAlgorithm* alg, RetransmissionContext* new_context) {
    assert(alg != NULL);

    list* visited = list_init();
    destroyRetransmissionContext(alg->r_context, visited);
    list_delete(visited);

    alg->r_context = new_context;
}

RetransmissionDelay* BA_getRetransmissionDelay(BroadcastAlgorithm* alg) {
    assert(alg != NULL);

    return alg->r_delay;
}

void BA_setRetransmissionDelay(BroadcastAlgorithm* alg, RetransmissionDelay* new_delay) {
    assert(alg != NULL);

    list* visited = list_init();
    destroyRetransmissionDelay(alg->r_delay, visited);
    list_delete(visited);

    alg->r_delay = new_delay;
}

RetransmissionPolicy* BA_getRetransmissionPolicy(BroadcastAlgorithm* alg) {
    assert(alg != NULL);

    return alg->r_policy;
}

void BA_setRetransmissionPolicy(BroadcastAlgorithm* alg, RetransmissionPolicy* new_policy) {
    assert(alg != NULL);

    list* visited = list_init();
    destroyRetransmissionPolicy(alg->r_policy, visited);
    list_delete(visited);

    alg->r_policy = new_policy;
}

unsigned int BA_getRetransmissionPhases(BroadcastAlgorithm* alg) {
    assert(alg != NULL);

    return alg->n_phases;
}

void BA_setRetransmissionPhases(BroadcastAlgorithm* alg, unsigned int phases) {
    assert(alg != NULL);

    alg->n_phases = phases;
}

unsigned long BA_computeRetransmissionDelay(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned long remaining, bool isCopy, unsigned char* myID) {
    assert(algorithm != NULL && p_msg != NULL);

    RetransmissionDelay* rd = algorithm->r_delay;
    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
	unsigned long result = RD_compute(rd, p_msg, remaining, isCopy, myID, rc, visited);
    list_delete(visited);

    return result;
}

bool BA_evalRetransmissionPolicy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID) {
    assert(algorithm != NULL);

    RetransmissionPolicy* rp = algorithm->r_policy;
    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
	bool result = RP_eval(rp, p_msg, myID, rc, visited);
    list_delete(visited);

    return result;
}

void BA_initRetransmissionContext(BroadcastAlgorithm* algorithm, proto_def* protocol_definition, unsigned char* myID) {
    assert(algorithm != NULL);

    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
    RC_init(rc, protocol_definition, myID, visited);
    list_delete(visited);
}

void BA_processEvent(BroadcastAlgorithm* algorithm, queue_t_elem* event, unsigned char* myID) {
    assert(algorithm != NULL);

    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
    RC_processEvent(rc, event, myID, visited);
    list_delete(visited);
}

unsigned int BA_createHeader(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, void** context_header, unsigned char* myID) {
    assert(algorithm != NULL && p_msg != NULL);

    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
    unsigned int result = RC_createHeader(rc, p_msg, context_header, myID, visited);
    list_delete(visited);

    return result;
}

void BA_processCopy(BroadcastAlgorithm* algorithm, PendingMessage* p_msg, unsigned char* myID) {
    assert(algorithm != NULL && p_msg != NULL);

    RetransmissionContext* rc = algorithm->r_context;

    list* visited = list_init();
    RC_processCopy(rc, p_msg, myID, visited);
    list_delete(visited);
}
