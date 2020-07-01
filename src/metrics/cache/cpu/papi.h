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
