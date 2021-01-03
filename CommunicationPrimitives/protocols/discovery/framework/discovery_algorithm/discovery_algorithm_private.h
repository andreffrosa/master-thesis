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

#ifndef _DISCOVERY_ALGORITHM_PRIVATE_H_
#define _DISCOVERY_ALGORITHM_PRIVATE_H_

#include "discovery_algorithm.h"

#include "discovery_pattern/discovery_pattern_private.h"
#include "discovery_period/discovery_period_private.h"
#include "discovery_context/discovery_context_private.h"
#include "link_quality/link_quality_private.h"
#include "link_admission/link_admission_private.h"

typedef struct _DiscoveryAlgorithm {
    DiscoveryPattern* d_pattern;
    DiscoveryPeriod* d_period;
    LinkQuality* lq_metric;
    LinkAdmission* la_policy;
    DiscoveryContext* d_context;
} DiscoveryAlgorithm;

#endif /* _DISCOVERY_ALGORITHM_PRIVATE_H_ */
