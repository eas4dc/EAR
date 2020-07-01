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

#ifndef _ARCH_INFO_H_
#define _ARCH_INFO_H_

#include <common/sizes.h>
#include <common/states.h>
#include <common/hardware/hardware_info.h>

#define MAX_FREQ_AVX2 2600000
#define MAX_FREQ_AVX512 2200000

typedef struct architecture{
	topology_t 		top;
	unsigned long max_freq_avx512;
	unsigned long max_freq_avx2;
	int 					pstates;
}architecture_t;
/** Fills the current architecture in arch*/
state_t get_arch_desc(architecture_t *arch);

/** Copy src in dest */
state_t copy_arch_desc(architecture_t *dest,architecture_t *src);

/** Prints in stdout the current architecture*/
void print_arch_desc(architecture_t *arch);

/** Uses the verbose to print the current architecture*/
void verbose_architecture(int v, architecture_t *arch);

#endif
