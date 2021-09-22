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

#ifndef _STALL_METRICS_H_
#define _STALL_METRICS_H_

#include <metrics/common/papi.h>

// Stall cycles
/** Initializes the event metrics for stall cycles. */
int init_stall_metrics();

/** Resets the metrics for stall cycles. */
void reset_stall_metrics();

/** Starts tracking stall cycle metrics. */
void start_stall_metrics();

/** Stops stall metrics and accumulates registered stall cycles to the
*   variable recieved by parameter. */
void stop_stall_metrics(long long *stall_cycles);

/** Puts the current stall cycles metrics in the variable recieved by parameter. */
void get_stall_metrics(long long *total_stall_cycles);

#endif
