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

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common/hardware/hardware_info.h>

static int file_is_accessible(const char *path)
{
	return (access(path, F_OK) == 0);
}

int detect_packages(int **mypackage_map) 
{
	char filename[BUFSIZ];
	FILE *fff;
	int package;
	int *package_map;
	int i;
	int num_cpus,num_cores, num_packages = 0;
    topology_t topo;
    hardware_gettopology(&topo);
    num_cpus = topo.sockets;
    num_cores = topo.cores*topo.sockets;
    package_map = calloc(num_cores, sizeof(int));
    
	if (num_cpus < 1 || num_cores < 1) {
        return 0;
	}

	for(i=0;i<num_cores;i++) {
		package_map[i]=-1;
	}

	for(i=0;i<num_cores;i++)
	{
		sprintf(filename,"/sys/devices/system/cpu/cpu%d/topology/physical_package_id",i);
		fff=fopen(filename,"r");
		if (fff==NULL) break;
		fscanf(fff,"%d",&package);
		fclose(fff);

		if (package_map[package]==-1) {
		    num_packages++;
	    	package_map[package]=i;
	    }
	}
	if (mypackage_map!=NULL) *mypackage_map=package_map;
	else free(package_map);

	return num_packages;
}


static state_t hardware_sibling_read(const char *path, int *result)
{
	char cbuf[2];
	ssize_t r;
	long n;
	int fd;

	if ((fd = open(path, F_RD)) < 0) {
		state_return_msg(EAR_OPEN_ERROR, errno, strerror(errno));
	}

	while((r = read(fd, cbuf, 1)) > 0)
	{
		cbuf[1] = '\0';

		if (isxdigit(cbuf[0])) {
			for (n = strtol(cbuf, NULL, 16); n > 0; n /= 2) {
				if (n % 2) *result = *result + 1;
			}
		}
	}

	close(fd);

	if (r < 0) {
		state_return_msg(EAR_READ_ERROR, errno, strerror(errno));
	}

	state_return(EAR_SUCCESS);
}

state_t hardware_topology_getsize(uint *size)
{
	topology_t topo;
	state_t s;

	s = hardware_gettopology(&topo);

	if (state_fail(s)) {
		return s;
	}

	*size = topo.sockets * topo.cores * topo.threads;

	return EAR_SUCCESS;
}

state_t hardware_gettopology(topology_t *topo)
{
	char path[SZ_NAME_LARGE];
	int aux1 = -1;
	int aux2 = -1;
	state_t s;

	topo->cores = 0;
	topo->threads = 0;
	topo->sockets = 0;
	topo->numas = 0;


	/* Number of CPUs */
	do {
		aux1 += 1;
		sprintf(path, PATH_SYS_SYSTEM "/cpu/cpu%d", aux1);
	} while(file_is_accessible(path));

	/* Number of threads per core */
	if (file_is_accessible(PATH_SYS_CPU0_TOTH)) {
		s = hardware_sibling_read(PATH_SYS_CPU0_TOTH, &topo->threads);

		if (state_fail(s)) {
			return s;
		}
	}

	/* Number of cores per socket */
	if (file_is_accessible(PATH_SYS_CPU0_TOCO) && topo->threads > 0) {
		s = hardware_sibling_read(PATH_SYS_CPU0_TOCO, &topo->cores);

		if (state_fail(s)) {
			return s;
		}

		topo->cores /= topo->threads;
	}

	if (topo->cores <= 0) {
		return s;
	}

	/* Number of CPUs per numa node */
	do {
		aux2 += 1;
		sprintf(path, PATH_SYS_SYSTEM "/node/node%d", topo->cores);
	} while(file_is_accessible(path));

	if (aux2 > 1) {
		aux2 = aux1 / aux2;
	} else {
		aux2 = aux1;
	}

	/* Number of sockets */
	if (topo->threads > 0 && topo->cores > 0) {
		topo->sockets = aux2 / topo->cores / topo->threads;
	}

	return EAR_SUCCESS;
}

int is_cpu_examinable()
{
    char vendor_id[16];

    get_vendor_id(vendor_id);

    if(strcmp(INTEL_VENDOR_NAME, vendor_id) != 0) {
        return EAR_ERROR;
	}
	if (!cpuid_isleaf(11)) {
		return EAR_WARNING;
	}
    return EAR_SUCCESS;
}

int get_vendor_id(char *vendor_id)
{
    cpuid_regs_t r;
	int *pointer = (int *) vendor_id;

	CPUID(r,0,0);
	pointer[0] = r.ebx;
	pointer[1] = r.edx;
	pointer[2] = r.ecx;
	return 1;
}

int get_family()
{
    cpuid_regs_t r;
	CPUID(r,1,0);
	return cpuid_getbits(r.eax, 11, 8);
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

int get_cache_line_size()
{
    unsigned int max_level = 0;
    unsigned int line_size = 0;
	unsigned int level = 0;
	cpuid_regs_t r;
	int index = 0;

	while (1)
	{
		CPUID(r,4,index);

		if (!(r.eax & 0x0F)) return line_size;
        level = cpuid_getbits(r.eax, 7, 5);

        if (level >= max_level) {
            max_level = level;
            line_size = cpuid_getbits(r.ebx, 11, 0) + 1;
		}

		index = index + 1;
	}
}

int is_cpu_hyperthreading_capable()
{
    cpuid_regs_t r;
    //
    CPUID(r,1,0);
    //
    return cpuid_getbits(r.edx, 28, 28);
}

// References:
// 	- Intel System Programming Guide
//    Vol. 2A, Software Developer Manual
//	  3-193, INSTRUCTION SET REFERENCE, A-L
// 	  Thermal and Power Management Leaf
int is_cpu_boost_enabled()
{
	cpuid_regs_t r;
	//
	CPUID(r,6,0);
	//
	return cpuid_getbits(r.eax, 1, 1);
}

