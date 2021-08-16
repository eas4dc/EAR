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

#include <string.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/sizes.h>
#include <common/states.h>
//#include <common/hardware/cpupower.h>
#include <common/hardware/architecture.h>

/** Fills the current architecture in arch*/
state_t get_arch_desc(architecture_t *arch)
{
	state_t ret = EAR_SUCCESS;
	if (arch==NULL) return EAR_ERROR;
	arch->pstates = 0;
	topology_init(&arch->top);
	return ret;	
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
		return;
	}
	
}

/** Uses the verbose to print the current architecture*/
void verbose_architecture(int v, architecture_t *arch)
{
	if (arch==NULL){ 
		verbose(v,"arch NULL pointer\n");
		return;
	}
	verbose(v,"max avx512 %lu max freq for avx2 instructions %lu num pstates %d\n",arch->max_freq_avx512,
	arch->max_freq_avx2,arch->pstates);
}

