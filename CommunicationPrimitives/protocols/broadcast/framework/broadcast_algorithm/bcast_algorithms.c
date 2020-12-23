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

#include "bcast_algorithms.h"

#include <assert.h>

BroadcastAlgorithm* Flooding(unsigned long t) {
	return newBroadcastAlgorithm(new_list(1, EmptyContext()), RandomDelay(t), TruePolicy(), 1);
}

BroadcastAlgorithm* Gossip1(unsigned long t, double p) {
	return newBroadcastAlgorithm(new_list(1, EmptyContext()), RandomDelay(t), ProbabilityPolicy(p), 1);
}

BroadcastAlgorithm* Gossip1Horizon(unsigned long t, double p, unsigned int k) {
	return newBroadcastAlgorithm(new_list(1, HopsContext("first")), RandomDelay(t), HorizonProbabilityPolicy(p, k), 1);
}

BroadcastAlgorithm* Gossip2(unsigned long t, double p1, unsigned int k, double p2, unsigned int n) {
    BroadcastAlgorithm* alg = newBroadcastAlgorithm(
        new_list(2, HopsContext("first"), NeighborsContext()),
        RandomDelay(t),
        Gossip2Policy(p1, k, p2, n),
        1
    );
	return alg;
}

BroadcastAlgorithm* RAPID(unsigned long t, double beta) {
	return newBroadcastAlgorithm(new_list(1, NeighborsContext()), RandomDelay(t), RapidPolicy(beta), 1);
}

BroadcastAlgorithm* EnhancedRAPID(unsigned long t1, unsigned long t2, double beta) {
	return newBroadcastAlgorithm(new_list(1, NeighborsContext()), TwoPhaseRandomDelay(t1, t2), EnhancedRapidPolicy(beta), 2);
}

BroadcastAlgorithm* Gossip3(unsigned long t1, unsigned long t2, double p, unsigned int k, unsigned int m) {
	return newBroadcastAlgorithm(
        new_list(1, HopsContext("first")),
        TwoPhaseRandomDelay(t1, t2),
        Gossip3Policy(p, k, m),
        2
    );
}

BroadcastAlgorithm* Counting(unsigned long t, unsigned int c) {
	return newBroadcastAlgorithm(new_list(1, EmptyContext()), RandomDelay(t), CountPolicy(c), 1);
}

BroadcastAlgorithm* CountingParents(unsigned long t, unsigned int c, bool count_same_parent) {
	return newBroadcastAlgorithm(new_list(1, ParentsContext(1)), RandomDelay(t), CountParentsPolicy(c, count_same_parent), 1);
}

BroadcastAlgorithm* HopCountAided(unsigned long t) {
	return newBroadcastAlgorithm(new_list(1, HopsContext("first")), RandomDelay(t), HopCountAidedPolicy(), 1);
}

BroadcastAlgorithm* SBA(unsigned long t) {
    return newBroadcastAlgorithm(new_list(1, NeighborsContext()), SBADelay(t), SBAPolicy(), 1);
}

BroadcastAlgorithm* LENWB(unsigned long t) {
    BroadcastAlgorithm* alg = newBroadcastAlgorithm(
        new_list(2, NeighborsContext(), LENWBContext()),
        RandomDelay(t),
        LENWBPolicy(),
        1
    );
    return alg;
}

BroadcastAlgorithm* NABA1(unsigned long t, unsigned int c) {
	return newBroadcastAlgorithm(new_list(1, NeighborsContext()), DensityNeighDelay(t), NeighborCountingPolicy(c), 1);
}

BroadcastAlgorithm* NABA2(unsigned long t, unsigned int c1, unsigned int c2) {
	return newBroadcastAlgorithm(new_list(1, NeighborsContext()), DensityNeighDelay(t), PbNeighCountingPolicy(c1, c2), 1);
}

BroadcastAlgorithm* NABA3(unsigned long t) {
    BroadcastAlgorithm* alg = newBroadcastAlgorithm(
        new_list(2, NeighborsContext(), LabelNeighsContext()),
        DensityNeighDelay(t),
        CriticalNeighPolicy(false, true, 0.0),
        1
    );
    return alg;
}

BroadcastAlgorithm* NABA4(unsigned long t) {
    BroadcastAlgorithm* alg = newBroadcastAlgorithm(
        new_list(2, NeighborsContext(), LabelNeighsContext()),
        DensityNeighDelay(t),
        CriticalNeighPolicy(false, true, 1.0),
        2
    );
    return alg;
}

BroadcastAlgorithm* NABA3e4(unsigned long t, unsigned int np) {
	BroadcastAlgorithm* alg = newBroadcastAlgorithm(
        new_list(2, NeighborsContext(), LabelNeighsContext()),
        DensityNeighDelay(t),
        CriticalNeighPolicy(false, true, 1.0),
        np
    );
    return alg;
}

BroadcastAlgorithm* MPR(unsigned long t) {
    BroadcastAlgorithm* alg = newBroadcastAlgorithm(
        new_list(2, NeighborsContext(), MultiPointRelayContext()),
        RandomDelay(t),
        DelegatedNeighborsPolicy(),
        1
    );
    return alg;
}

BroadcastAlgorithm* AHBP(unsigned long t, unsigned int route_max_len, bool mobility_extension) {
    BroadcastAlgorithm* alg = newBroadcastAlgorithm(
        new_list(3, NeighborsContext(), RouteContext(route_max_len), AHBPContext()),
        RandomDelay(t),
        AHBPPolicy(mobility_extension),
        1
    );
    return alg;
}

BroadcastAlgorithm* DynamicProbability(double p, double p_l, double p_u, double d, unsigned long t1, unsigned long t2) {
    return newBroadcastAlgorithm(new_list(1, DynamicProbabilityContext(p, p_l, p_u, d, t1)), RandomDelay(t2), DynamicProbabilityPolicy(), 1);
}

BroadcastAlgorithm* RADExtension(unsigned long delta_t, unsigned int c) {
    return newBroadcastAlgorithm(new_list(1, EmptyContext()), RADExtensionDelay(delta_t), CountPolicy(c), 1);
}

BroadcastAlgorithm* HopCountAwareRADExtension(unsigned long delta_t, unsigned int c) {
    return newBroadcastAlgorithm(new_list(1, HopCountAwareRADExtensionContext(delta_t)), HopCountAwareRADExtensionDelay(delta_t), CountPolicy(c), 1);
}
