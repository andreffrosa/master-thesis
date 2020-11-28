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
BroadcastAlgorithm* Gossip1_hops(unsigned long t, double p, unsigned int k);
BroadcastAlgorithm* Gossip2(unsigned long t, double p1, unsigned int k, double p2, unsigned int n, unsigned int window_size, topology_discovery_args* d_args);
BroadcastAlgorithm* RAPID(unsigned long t, double beta, unsigned int window_size, topology_discovery_args* d_args);
BroadcastAlgorithm* EnhancedRAPID(unsigned long t1, unsigned long t2, double beta, unsigned int window_size, topology_discovery_args* d_args);
BroadcastAlgorithm* Gossip3(unsigned long t1, unsigned long t2, double p, unsigned int k, unsigned int m);
BroadcastAlgorithm* Counting(unsigned long t, unsigned int c);
BroadcastAlgorithm* HopCountAided(unsigned long t);

BroadcastAlgorithm* SBA(unsigned long t, unsigned int window_size, topology_discovery_args* d_args);
BroadcastAlgorithm* LENWB(unsigned long t, unsigned int window_size, topology_discovery_args* d_args);

BroadcastAlgorithm* NABA1(unsigned long t, unsigned int c, unsigned int window_size, topology_discovery_args* d_args); // CountingNABA
BroadcastAlgorithm* NABA2(unsigned long t, unsigned int c1, unsigned int c2, unsigned int window_size, topology_discovery_args* d_args); // PbCountingNABA
BroadcastAlgorithm* NABA3(unsigned long t, unsigned int window_size, topology_discovery_args* d_args);
BroadcastAlgorithm* NABA4(unsigned long t, double min_critical_coverage, unsigned int window_size, topology_discovery_args* d_args);
BroadcastAlgorithm* NABA3e4(unsigned long t, double min_critical_coverage, unsigned int np, unsigned int window_size, topology_discovery_args* d_args); // CriticalNABA

BroadcastAlgorithm* MPR(unsigned long t, double hyst_threshold_low, double hyst_threshold_high, unsigned int window_size, topology_discovery_args* d_args);
BroadcastAlgorithm* AHBP(int ex, unsigned long t, double hyst_threshold_low, double hyst_threshold_high, unsigned int route_max_len, unsigned int window_size, topology_discovery_args* d_args);

BroadcastAlgorithm* DynamicProbability(double p, double p_l, double p_u, double d, unsigned long t1, unsigned long t2);

BroadcastAlgorithm* RADExtension(unsigned long delta_t, unsigned int c);
BroadcastAlgorithm* HopCountAwareRADExtension(unsigned long delta_t, unsigned int c);

#endif /* _BCAST_ALGORITHMS_H_ */
