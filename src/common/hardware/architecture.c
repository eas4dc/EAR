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

#include <string.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/hardware/hardware_info.h>
#include <common/hardware/cpupower.h>
#include <common/hardware/architecture.h>

/** Fills the current architecture in arch*/
state_t get_arch_desc(architecture_t *arch)
{
	state_t ret;
	if (arch==NULL) return EAR_ERROR;
	ret=hardware_gettopology(&arch->top);
	if (ret!=EAR_SUCCESS) return ret;
	arch->max_freq_avx512=MAX_FREQ_AVX512;
	arch->max_freq_avx2=MAX_FREQ_AVX2;
	arch->pstates=CPUfreq_get_num_pstates(0);
	return EAR_SUCCESS;	
}

/** Copy src in dest */
state_t copy_arch_desc(architecture_t *dest,architecture_t *src)
{
	if ((dest==NULL) || (src==NULL)) return EAR_ERROR;
	memcpy(dest,src,sizeof(architecture_t));
	return EAR_SUCCESS;
}

/** Prints in stdout the current architecture*/
void print_arch_desc(architecture_t *arch)
{
	if (arch==NULL){ 
		printf("arch NULL pointer\n");
		return;
	}
	printf("cores %d threads %d sockets %d numas %d ", arch->top.cores,arch->top.threads,arch->top.sockets,arch->top.numas);
	printf("max avx512 %lu max freq for avx2 instructions %lu num pstates %d\n",arch->max_freq_avx512,
	arch->max_freq_avx2,arch->pstates);
	
}

/** Uses the verbose to print the current architecture*/
void verbose_architecture(int v, architecture_t *arch)
{
	if (arch==NULL){ 
		verbose(v,"arch NULL pointer\n");
		return;
	}
	verbose(v,"cores %d threads %d sockets %d numas %d ", arch->top.cores,arch->top.threads,arch->top.sockets,arch->top.numas);
	verbose(v,"max avx512 %lu max freq for avx2 instructions %lu num pstates %d\n",arch->max_freq_avx512,
	arch->max_freq_avx2,arch->pstates);
}

