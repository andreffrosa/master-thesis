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

#ifndef _DISCOVERY_FRAMEWORK_LINK_QUALITY_H_
#define _DISCOVERY_FRAMEWORK_LINK_QUALITY_H_

#include "data_structures/list.h"

typedef struct _LinkQuality LinkQuality;

void destroyLinkQualityMetric(LinkQuality* lqm/*, list* visited*/);

double LQ_compute(LinkQuality* lqm, void* lq_attrs, double previous_link_quality, unsigned int lost, unsigned int received, bool init, struct timespec* current_time);

void* LQ_createAttrs(LinkQuality* lqm);

void LQ_destroyAttrs(LinkQuality* lqm, void* lq_attrs);

LinkQuality* SMALinkQuality(double initial_quality,  unsigned int n_buckets, unsigned int bucket_duration_s);

LinkQuality* WMALinkQuality(double initial_quality,  unsigned int n_buckets, unsigned int bucket_duration_s);

LinkQuality* EMALinkQuality(double initial_quality, double scalling,  unsigned int n_buckets, unsigned int bucket_duration_s);

LinkQuality* SlidingWindowLinkQuality(double initial_quality, unsigned int window_size);

#endif /* _DISCOVERY_FRAMEWORK_LINK_QUALITY_H_ */
