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

#ifndef _FLOPS_METRICS_H_
#define _FLOPS_METRICS_H_

#include <metrics/common/papi.h>

/** Initializes the event metrics for flops. */
int init_flops_metrics();

/** Resets the metrics for flops */
void reset_flops_metrics();

/** Starts tracking flops metrics. */
void start_flops_metrics();

/** Stops flops metrics and accumulates registered flops and number of operations
*   to the variables recieved by parameter. */
void stop_flops_metrics(long long *total_flops, long long *f_operations);

/** Returns the total number of fops events. */
int get_number_fops_events();

/** Puts the current flops metrics in the variable recieved by parameter. */
void get_total_fops(long long *metrics);

/** Returns the amount of floating point operations done in the time total_timei,
*   in GFlops. */
double gflops(ulong total_timei, uint total_cores);

/** Puts the weights of the fops instructions into *weight_vector. */
void get_weigth_fops_instructions(int *weigth_vector);

/** Prints the total GFlops per process and node if verbosity level is at 1 or
*   higher. */
void print_gflops(long long total_inst, ulong total_time, uint total_cores);

#endif
