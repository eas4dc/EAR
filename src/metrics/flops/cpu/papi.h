/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
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

#else
#endif
