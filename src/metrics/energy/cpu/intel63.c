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

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <common/config.h>
#include <common/states.h>
//#define SHOW_DEBUGS 0

#include <common/output/verbose.h>
#include <common/math_operations.h>
#include <common/hardware/hardware_info.h>

#include <metrics/energy/cpu.h>
#include <metrics/common/omsr.h>

/** Intel */
#define MSR_INTEL_RAPL_POWER_UNIT		0x606
/* PKG RAPL Domain */
//#define MSR_PKG_RAPL_POWER_LIMIT		0x610
#define MSR_INTEL_PKG_ENERGY_STATUS		0x611
//#define MSR_PKG_PERF_STATUS			0x613
//#define MSR_PKG_POWER_INFO			0x614
/* DRAM RAPL Domain */
//#define MSR_DRAM_POWER_LIMIT			0x618
#define MSR_INTEL_DRAM_ENERGY_STATUS	0x619
//#define MSR_DRAM_PERF_STATUS			0x61B
//#define MSR_DRAM_POWER_INFO			0x61C
/** AMD */
#define MSR_AMD_RAPL_POWER_UNIT			0xC0010299
#define MSR_AMD_PKG_ENERGY_STATUS		0xC001029B
#define MSR_AMD_CORE_ENERGY_STATUS		0xC001029A

//

static pthread_mutex_t rapl_msr_lock = PTHREAD_MUTEX_INITIALIZER;
static int rapl_msr_instances = 0;

static double cpu_energy_units;
static double dram_energy_units;
static int vendor;

int init_rapl_msr(int *fd_map)
{
	int j;
	ullong result;

	/* If it is not initialized, I do it, else, I get the ids */
	pthread_mutex_lock(&rapl_msr_lock);

	topology_t topo;
	topology_init(&topo);
	vendor = topo.vendor;

	if (is_msr_initialized() == 0) {
		debug("MSR registers already initialized");
		init_msr(fd_map);
	} else {
		get_msr_ids(fd_map);
	}
	
	rapl_msr_instances++;

	/* Ask for msr info */
	for (j = 0; j < get_total_packages(); j++)
	{
		off_t energy_status;

		if (vendor == VENDOR_INTEL) {
			energy_status = MSR_INTEL_RAPL_POWER_UNIT;
		} else {
			energy_status = MSR_AMD_RAPL_POWER_UNIT;
		}

		if (omsr_read(&fd_map[j], &result, sizeof result, energy_status)) {
			debug("Error in omsr_read init_rapl_msr");
			pthread_mutex_unlock(&rapl_msr_lock);
			return EAR_ERROR;
		}

		//power_units = pow(0.5, (double) (result & 0xf));
		//time_units = pow(0.5, (double) ((result >> 16) & 0xf));
		
		cpu_energy_units = pow(0.5, (double) ((result >> 8) & 0x1f));
		// Take a look to Intel's RAPL test
		dram_energy_units = pow(0.5, (double) 16);
	}
	pthread_mutex_unlock(&rapl_msr_lock);
	return EAR_SUCCESS;
}

/* DRAM 0, DRAM 1,..DRAM N, PCK0,PCK1,...PCKN */
int read_rapl_msr(int *fd_map, ullong *_values)
{
	ullong result;
	int j;
	int nump;
	nump = get_total_packages();

	for (j = 0; j < nump; j++)
	{
		off_t offset;

		/* PKG reading */
		if (vendor == VENDOR_INTEL) {
			offset = MSR_INTEL_PKG_ENERGY_STATUS;
		} else {
			offset = MSR_AMD_PKG_ENERGY_STATUS;
		}

		if (omsr_read(&fd_map[j], &result, sizeof result, offset)) {
			debug("Error in omsr_read read_rapl_msr");
			return EAR_ERROR;
		}
		result &= 0xffffffff;
		_values[nump + j] = (ullong) result * (cpu_energy_units * 1000000000);

		/* DRAM reading */
		if (vendor == VENDOR_INTEL) {
			offset = MSR_INTEL_DRAM_ENERGY_STATUS;
		} else {
			offset = MSR_AMD_CORE_ENERGY_STATUS;
		}

		if (omsr_read(&fd_map[j], &result, sizeof result, offset)) {
			debug("Error in omsr_read read_rapl_msr");
			return EAR_ERROR;
		}
		result &= 0xffffffff;

		if (vendor != VENDOR_AMD) {
			_values[j] = (ullong) result * (dram_energy_units * 1000000000);
		} else {
			_values[j] = 0;
		}
	}
	return EAR_SUCCESS;
}

void dispose_rapl_msr(int *fd_map)
{
	int j, tp;
	pthread_mutex_lock(&rapl_msr_lock);
	rapl_msr_instances--;
	if (rapl_msr_instances == 0) {
		tp = get_total_packages();
		for (j = 0; j < tp; j++) omsr_close(&fd_map[j]);
	}
	pthread_mutex_unlock(&rapl_msr_lock);
}

void diff_rapl_msr_energy(ullong *diff, ullong *end, ullong *init)
{
	ullong ret = 0;
	int nump, j;
	nump = get_total_packages();

	for (j = 0; j < nump * RAPL_ENERGY_EV; j++) {
		if (end[j] >= init[j]) {
			ret = end[j] - init[j];
		} else {
			ret = ullong_diff_overflow(init[j], end[j]);
		}
		diff[j] = ret;
	}
}

ullong acum_rapl_energy(ullong *values)
{
	ullong ret = 0;
	int nump, j;
	nump = get_total_packages();
	for (j = 0; j < nump * RAPL_ENERGY_EV; j++) {
		ret = ret + values[j];
	}
	return ret;
}

void rapl_msr_energy_to_str(char *b, ullong *values)
{
	int nump, j;
	char baux[512];
	nump = get_total_packages();

	sprintf(baux, ", CPU (");
	for (j = 0; j < nump; j++) {
		if (j < (nump - 1)) {
			sprintf(b, "%llu,", values[nump * RAPL_PCK_EV + j]);
		} else {
			sprintf(b, "%llu)", values[nump * RAPL_PCK_EV + j]);
		}
		strcat(baux, b);
	}

	sprintf(b, ", DRAM (");
	strcat(baux, b);
	for (j = 0; j < nump; j++) {
		if (j < (nump - 1)) {
			sprintf(b, "%llu,", values[nump * RAPL_DRAM_EV + j]);
		} else {
			sprintf(b, "%llu)", values[nump * RAPL_DRAM_EV + j]);
		}
		strcat(baux, b);
	}
	strcpy(b, baux);
}
