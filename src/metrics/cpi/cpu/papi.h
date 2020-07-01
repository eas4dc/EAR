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
