/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef _CACHE_METRICS_H_
#define _CACHE_METRICS_H_

#include <metrics/common/papi.h>

#define CACHE_SETS 3
#define CACHE_EVS 2


/** Initializes the event metrics for each cache level. */
int init_cache_metrics();

/** Resets the metrics for all cache levels. */
void reset_cache_metrics();

/** Starts tracking cache metrics. */
void start_cache_metrics();

/** Stops cache metrics and accumulates registered events in the variables corresponding
* 	to each cache level recieved by parameter. */
void stop_cache_metrics(long long *l1);


void print_cache_metrics(long long *L);
#else
#endif
