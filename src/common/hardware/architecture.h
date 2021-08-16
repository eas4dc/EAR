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

#ifndef _ARCH_INFO_H_
#define _ARCH_INFO_H_

#include <common/sizes.h>
#include <common/states.h>
#include <common/hardware/topology.h>

typedef struct architecture{
	unsigned long max_freq_avx512;
	unsigned long max_freq_avx2;
	int pstates;
	topology_t top;
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
