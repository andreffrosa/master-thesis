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

#ifndef _BCAST_ALGORITHMS_H_
#define _BCAST_ALGORITHMS_H_

#include "broadcast_algorithm.h"

BroadcastAlgorithm* Flooding(unsigned long t);
BroadcastAlgorithm* Gossip1(unsigned long t, double p);
BroadcastAlgorithm* Gossip1Horizon(unsigned long t, double p, unsigned int k);
BroadcastAlgorithm* Gossip2(unsigned long t, double p1, unsigned int k, double p2, unsigned int n);
BroadcastAlgorithm* RAPID(unsigned long t, double beta);
BroadcastAlgorithm* EnhancedRAPID(unsigned long t1, unsigned long t2, double beta);
BroadcastAlgorithm* Gossip3(unsigned long t1, unsigned long t2, double p, unsigned int k, unsigned int m);
BroadcastAlgorithm* Counting(unsigned long t, unsigned int c);
BroadcastAlgorithm* CountingParents(unsigned long t, unsigned int c, bool count_same_parent);
BroadcastAlgorithm* HopCountAided(unsigned long t);

BroadcastAlgorithm* DynamicProbability(double p, double p_l, double p_u, double d, unsigned long t1, unsigned long t2);

BroadcastAlgorithm* RADExtension(unsigned long delta_t, unsigned int c);
BroadcastAlgorithm* HopCountAwareRADExtension(unsigned long delta_t, unsigned int c);

BroadcastAlgorithm* NABA1(unsigned long t, unsigned int c); // CountingNABA
BroadcastAlgorithm* NABA2(unsigned long t, unsigned int c1, unsigned int c2); // PbCountingNABA
BroadcastAlgorithm* NABA3(unsigned long t);
BroadcastAlgorithm* NABA4(unsigned long t);
BroadcastAlgorithm* NABA3e4(unsigned long t, unsigned int np); // CriticalNABA

BroadcastAlgorithm* SBA(unsigned long t);
BroadcastAlgorithm* LENWB(unsigned long t);

BroadcastAlgorithm* MPR(unsigned long t);
BroadcastAlgorithm* AHBP(unsigned long t, unsigned int route_max_len, bool mobility_extension);

#endif /* _BCAST_ALGORITHMS_H_ */
