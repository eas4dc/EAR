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

#define _GNU_SOURCE
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <common/config.h>
#include <common/hardware/topology.h>
#include <common/hardware/hardware_info.h>

#define INTEL_VENDOR_NAME       "GenuineIntel"

uint num_cpus_in_mask(cpu_set_t *my_mask)
{
    uint c, lcpus = 0;

    for (c = 0; c < MAX_CPUS_SUPPORTED; c++){
        if (CPU_ISSET(c, my_mask)) lcpus++;
    }
    return lcpus;

}

int list_cpus_in_mask(cpu_set_t *my_mask, uint n_cpus, uint *cpu_list)
{
    if (my_mask == NULL || cpu_list == NULL || n_cpus < 1) return -1;

    uint c, lcpus = 0;

    for (c = 0; c < MAX_CPUS_SUPPORTED && lcpus < n_cpus; c++) {
        if (CPU_ISSET(c, my_mask)) {
            cpu_list[lcpus] = c;
            lcpus++;
        }
    }

    if (lcpus < n_cpus) {
        return -1;
    }

    return 0;

}

void aggregate_cpus_to_mask(cpu_set_t *dst, cpu_set_t *src)
{
    for (uint c=0; c < MAX_CPUS_SUPPORTED; c++){
      if (CPU_ISSET(c, src)) CPU_SET(c, dst);
  }

}

void remove_cpus_from_mask(cpu_set_t *dst, cpu_set_t *src)
{
  for (uint c=0; c < MAX_CPUS_SUPPORTED; c++){
      if (CPU_ISSET(c, src)) CPU_CLR(c, dst);
  }

}


/* Sets in dst CPUs not in src */
void cpus_not_in_mask(cpu_set_t *dst, cpu_set_t *src)
{
	CPU_ZERO(dst);
  for (uint c=0; c < MAX_CPUS_SUPPORTED; c++){
      if (!CPU_ISSET(c, src)) CPU_SET(c, dst);
  }

}


void print_affinity_mask(topology_t *topo) 
{
    cpu_set_t mask;
		int i;
		CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == -1) return;
    fprintf(stderr,"sched_getaffinity = ");
    for (i = 0; i < topo->cpu_count; i++) {
        fprintf(stderr,"CPU[%d]=%d ", i,CPU_ISSET(i, &mask));
    }
    fprintf(stderr,"\n");
}

state_t print_this_affinity_mask(cpu_set_t *mask)
{
    if (mask == NULL) return EAR_ERROR;
    fprintf(stderr,"mask affinity = ");
    for (int i = 0; i < MAX_CPUS_SUPPORTED; i++) {
        fprintf(stderr,"CPU[%d]=%d ", i,CPU_ISSET(i, mask));
    }
    fprintf(stderr,"\n");
    return EAR_SUCCESS;
}

state_t verbose_affinity_mask(cpu_set_t *mask, uint cpus)
{
    if (mask == NULL) return EAR_ERROR;
    verbosen(0,"mask affinity = ");
    for (int i = 0; i < cpus; i++) {
        verbosen(0,"CPU[%d]=%d ", i,CPU_ISSET(i, mask));
    }
    verbosen(0,"\n");
    return EAR_SUCCESS;
}


state_t is_affinity_set(topology_t *topo,int pid,int *is_set,cpu_set_t *my_mask)
{
	cpu_set_t mask;
	*is_set=0;
	CPU_ZERO(&mask);
	CPU_ZERO(my_mask);
	if (sched_getaffinity(pid,sizeof(cpu_set_t), &mask) == -1) return EAR_ERROR;
	memcpy(my_mask,&mask,sizeof(cpu_set_t));
	int i;
	for (i = 0; i < topo->cpu_count; i++) {
		if (CPU_ISSET(i, &mask)) {
			*is_set=1;
			return EAR_SUCCESS;	
		}
	}
	return EAR_SUCCESS;
}

state_t set_affinity(int pid,cpu_set_t *mask)
{
	int ret;
	ret=sched_setaffinity(pid,sizeof(cpu_set_t),mask);
	if (ret <0 ) return EAR_ERROR;
	else return EAR_SUCCESS;
}

state_t add_affinity(topology_t *topo, cpu_set_t *dest,cpu_set_t *src)
{
	state_t s = EAR_SUCCESS;
	if ((dest == NULL) || (src == NULL)) return EAR_ERROR;
	for (uint i = 0;i<topo->cpu_count; i++){
		if (CPU_ISSET(i,src)) CPU_SET(i,dest);
	}
	return s;	
}


state_t set_mask_all_ones(cpu_set_t *mask)
{
    for (int i = 0; i < MAX_CPUS_SUPPORTED; i++)
    {
        CPU_SET(i, mask);
    }

    return EAR_SUCCESS;
}

#if 0
static int file_is_accessible(const char *path)
{
	return (access(path, F_OK) == 0);
}
#endif

static topology_t topo1;
static topology_t topo2;
static int init;

int detect_packages(int **mypackage_map) 
{
	int num_cpus, num_cores, num_packages = 0;
	int *package_map;
	int i;

	if (!init)
	{
		topology_init(&topo1);
		topology_select(&topo1, &topo2, TPSelect.socket, TPGroup.merge, 0);
		init = 1;
	}
    
	num_cores    = topo1.core_count;
	num_packages = topo1.socket_count;
	num_cpus     = topo1.socket_count;
	
	if (num_cpus < 1 || num_cores < 1) {
        	return 0;
	}

	if (mypackage_map != NULL) {
		*mypackage_map = calloc(num_cores, sizeof(int));
		package_map = *mypackage_map;
	}

	for (i = 0; mypackage_map != NULL && i < topo2.cpu_count; ++i) {
		package_map[i] = topo2.cpus[i].id;
	}

	return num_packages;
}

int get_model()
{
    cpuid_regs_t r;
	int model_full;
	int model_low;

	CPUID(r,1,0);
	model_low  = cpuid_getbits(r.eax, 7, 4);
	model_full = cpuid_getbits(r.eax, 19, 16);
	return (model_full << 4) | model_low;
}

int is_aperf_compatible()
{
    cpuid_regs_t r;

	if (!cpuid_isleaf(11)) {
		return EAR_ERROR;
	}

	CPUID(r,6,0);
    return r.ecx & 0x01;
}

int is_cpu_boost_enabled()
{
	cpuid_regs_t r;
	//
	CPUID(r,6,0);
	//

	return cpuid_getbits(r.eax, 1, 1);
}

int cpu_isset(int cpu, cpu_set_t *mask)
{
    if (cpu < 0 || mask == NULL) {
        return -1;
    }
    return CPU_ISSET(cpu, mask);
}
