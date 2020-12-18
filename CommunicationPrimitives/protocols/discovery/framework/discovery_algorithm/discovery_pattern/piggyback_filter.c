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

#include "discovery_pattern_common.h"

#include "../../framework.h"

#include <assert.h>

PiggybackFilter* newPiggybackFilter(void* args, void* vars, piggyback_filter filter, piggyback_filter_destroy destroy) {
    PiggybackFilter* pf = malloc(sizeof(PiggybackFilter));

    pf->state.args = args;
    pf->state.vars = vars;
    pf->filter = filter;
    pf->destroy = destroy;

    return pf;
}

void destroyPiggybackFilter(PiggybackFilter* pf) {
    if(pf) {
        if(pf->destroy) {
            pf->destroy(&pf->state);
        }
        free(pf);
    }
}

PiggybackType evalPiggybackFilter(PiggybackFilter* pf, YggMessage* msg, void* extra_args) {
    assert(pf);

    if(pf->filter) {
        return pf->filter(&pf->state, msg, extra_args);
    } else {
        return NO_PIGGYBACK;
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////

PiggybackFilter* NoPiggyback() {
    return newPiggybackFilter(NULL, NULL, NULL, NULL);
}

static PiggybackType PiggybackOnUnicast_(ModuleState* m_state, YggMessage* msg, void* extra_args) {
    bool unicast_msg = is_unicast_message(msg);
    return unicast_msg ? UNICAST_PIGGYBACK : NO_PIGGYBACK;
}

PiggybackFilter* PiggybackOnUnicast() {
    return newPiggybackFilter(NULL, NULL, &PiggybackOnUnicast_, NULL);
}

static PiggybackType PiggybackOnBroadcast_(ModuleState* m_state, YggMessage* msg, void* extra_args) {
    bool unicast_msg = is_unicast_message(msg);
    return !unicast_msg ? BROADCAST_PIGGYBACK : NO_PIGGYBACK;
}

PiggybackFilter* PiggybackOnBroadcast() {
    return newPiggybackFilter(NULL, NULL, &PiggybackOnBroadcast_, NULL);
}

static PiggybackType PiggybackOnDiscovery_(ModuleState* m_state, YggMessage* msg, void* extra_args) {
    bool unicast_msg = is_unicast_message(msg);
    return msg->Proto_id == DISCOVERY_FRAMEWORK_PROTO_ID && !unicast_msg ? BROADCAST_PIGGYBACK : NO_PIGGYBACK;
}

PiggybackFilter* PiggybackOnDiscovery() {
    return newPiggybackFilter(NULL, NULL, &PiggybackOnDiscovery_, NULL);
}

static PiggybackType PiggybackOnAll_(ModuleState* m_state, YggMessage* msg, void* extra_args) {
    bool convert_to_broadcast = *((bool*)m_state->args);
    bool unicast_msg = is_unicast_message(msg);
    return convert_to_broadcast ? BROADCAST_PIGGYBACK : (unicast_msg ? UNICAST_PIGGYBACK : BROADCAST_PIGGYBACK);
}

static void PiggybackOnAll_destroy(ModuleState* state) {
    free(state->args);
}

PiggybackFilter* PiggybackOnAll(bool convert_to_broadcast) {
    bool* arg = malloc(sizeof(bool));
    *arg = convert_to_broadcast;
    return newPiggybackFilter(arg, NULL, &PiggybackOnAll_, &PiggybackOnAll_destroy);
}

static PiggybackType PiggybackOnNewNeighbor_(ModuleState* m_state, YggMessage* msg, void* extra_args) {
    bool new_neighbor = extra_args ? *((bool*)extra_args) : false;
    return msg->Proto_id == DISCOVERY_FRAMEWORK_PROTO_ID && new_neighbor ? BROADCAST_PIGGYBACK : NO_PIGGYBACK;
}

PiggybackFilter* PiggybackOnNewNeighbor() {
    return newPiggybackFilter(NULL, NULL, &PiggybackOnNewNeighbor_, NULL);
}
















// TODO: o all tem a opção de converter unicast para bcast e o discovery tem de ter algo parecido porque os hacks podem ser unicast
