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

#ifndef _RAPL_METRICS_H_
#define _RAPL_METRICS_H_

#include <metrics/common/msr.h>

#define RAPL_POWER_EVS            2
#define RAPL_DRAM0          0
#define RAPL_DRAM1          1
#define RAPL_PACKAGE0       2
#define RAPL_PACKAGE1       3

/** Opens the necessary fds to read the MSR registers. Returns 0 on success
* 	and -1 on error. */
int init_rapl_msr(int *fd_map);

/** */
void dispose_rapl_msr(int *fd_map);

/** Reads rapl counters and stores them in values array. Returns 0 on success 
*	and -1 on error. */
int read_rapl_msr(int *fd_map,unsigned long long *_values);

#endif
