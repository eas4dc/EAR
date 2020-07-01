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

#ifndef METRICS_BANDWIDTH_CPU
#define METRICS_BANDWIDTH_CPU

#include <metrics/bandwidth/cpu/intel_haswell.h>

// All these functions returns the specific funcion errror.
// pmons.init for init_uncores etc.

/** Init the uncore counters for an specific cpu model number. */
int init_uncores(int cpu_model);

/** Get the number of performance monitor counters.
*   init_uncore_reading() have to be called before to scan the buses. */
int count_uncores();

/** Checks the state of the system uncore functions.
*   Returns EAR_SUCCESS, EAR_ERROR or EAR_WARNING. */
int check_uncores();

/** Freezes and resets all performance monitor (PMON) uncore counters. */
int reset_uncores();

/** Unfreezes all PMON uncore counters. */
int start_uncores();

/** Freezes all PMON uncore counters and gets it's values. The array
*   has to be greater or equal than the number of PMON uncore counters
*   returned by count_uncores() function. The returned values are the
*   read and write bandwith values in index [i] and [i+1] respectively. */
int stop_uncores(unsigned long long *values);

/** Gets the PMON uncore counters values. */
int read_uncores(unsigned long long *values);

/** Closes file descriptors and frees memory. */
int dispose_uncores();

#endif
