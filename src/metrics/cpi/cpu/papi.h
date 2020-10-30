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

#ifndef _BASIC_METRICS_H_
#define _BASIC_METRICS_H_

#include <metrics/common/papi.h>

// BASIC metrics are CYCLES and INSTRUCTIONS
/** Initializes the event metrics for cycles and instructions. */
int init_basic_metrics();

/** Resets the event metrics for both cycles and instructions. */
void reset_basic_metrics();

/** Starts tracking cycles and instructions metrics. */
void start_basic_metrics();

/** Stops tracking cycles and instruction metrics and accumulates said metrics
*   into the respective variables recieved by parameter. */
void stop_basic_metrics(long long *cycles, long long *instructions);

/** Puts the current cycles and instruction metrics in the variable recieved by parameter. */
void get_basic_metrics(long long *total_cycles, long long *instructions);

#endif
