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

#ifndef _RETRANSMISSION_POLICY_H_
#define _RETRANSMISSION_POLICY_H_

#include "../common.h"
#include "../retransmission_context/retransmission_context.h"

typedef struct _RetransmissionPolicy RetransmissionPolicy;

bool RP_eval(RetransmissionPolicy* r_policy, PendingMessage* p_msg, unsigned char* myID, hash_table* contexts);

//void RP_addDependency(RetransmissionPolicy* r_policy, char* dependency);

list* RP_getDependencies(RetransmissionPolicy* r_policy);

void destroyRetransmissionPolicy(RetransmissionPolicy* policy);

///////////////////////////////////////////////////////////////////////////////////////////////////////7

RetransmissionPolicy* TruePolicy();
RetransmissionPolicy* ProbabilityPolicy(double p);
RetransmissionPolicy* CountPolicy(unsigned int c);

RetransmissionPolicy* CountParentsPolicy(unsigned int c, bool count_same_parents);

RetransmissionPolicy* NeighborCountingPolicy(unsigned int c);
RetransmissionPolicy* PbNeighCountingPolicy(unsigned int c1, unsigned int c2);

RetransmissionPolicy* CriticalNeighPolicy(bool all_copies, bool mobility_extension, double min_critical_coverage);

RetransmissionPolicy* EnsemblePolicy(bool (*ensemble_function)(bool* values, int amount), int amount, ...);

// Estas politicas bem como o gossip não metem um 1º delay aleatório, só o enhanced rapid
RetransmissionPolicy* HorizonProbabilityPolicy(double p, unsigned int k);
RetransmissionPolicy* Gossip2Policy(double p1, unsigned int k, double p2, unsigned int n);
RetransmissionPolicy* RapidPolicy(double beta);
RetransmissionPolicy* EnhancedRapidPolicy(double beta);
RetransmissionPolicy* Gossip3Policy(double p, unsigned int k, unsigned int m);
RetransmissionPolicy* HopCountAidedPolicy();

RetransmissionPolicy* SBAPolicy();
RetransmissionPolicy* LENWBPolicy();

RetransmissionPolicy* DelegatedNeighborsPolicy();

RetransmissionPolicy* AHBPPolicy(bool mobility_extension);

RetransmissionPolicy* DynamicProbabilityPolicy();

RetransmissionPolicy* BiFloodingPolicy();

RetransmissionPolicy* BATMANPolicy();

#endif /* _RETRANSMISSION_POLICY_H_ */
