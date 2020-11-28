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

typedef struct _RetransmissionPolicy RetransmissionPolicy;

void destroyRetransmissionPolicy(RetransmissionPolicy* policy, list* visited);

RetransmissionPolicy* TruePolicy();
RetransmissionPolicy* ProbabilityPolicy(double p);
RetransmissionPolicy* CountPolicy(unsigned int c);

RetransmissionPolicy* NeighborCountingPolicy(unsigned int c);
RetransmissionPolicy* PbNeighCountingPolicy(unsigned int c1, unsigned int c2);

RetransmissionPolicy* CriticalNeighPolicy(double min_critical_coverage);

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

// ex = 0 -> convetional; ex = 1 -> retransmits if parent is not known; ex = 2 -> retansmits if 1 or if current node is not know to parent
RetransmissionPolicy* AHBPPolicy(int ex);

RetransmissionPolicy* DynamicProbabilityPolicy();

#endif /* _RETRANSMISSION_POLICY_H_ */
