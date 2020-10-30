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
#include <common/hardware/hardware_info.h>
#include <common/math_operations.h>
#include <metrics/energy/energy_cpu.h>
#include <metrics/common/omsr.h>



#define MSR_INTEL_RAPL_POWER_UNIT		0x606

/* PKG RAPL Domain */
#define MSR_PKG_RAPL_POWER_LIMIT		0x610 
#define MSR_INTEL_PKG_ENERGY_STATUS		0x611
#define MSR_PKG_PERF_STATUS				0x613
#define MSR_PKG_POWER_INFO				0x614 

/* DRAM RAPL Domain */
#define MSR_DRAM_POWER_LIMIT			0x618
#define MSR_DRAM_ENERGY_STATUS			0x619
#define MSR_DRAM_PERF_STATUS			0x61B
#define MSR_DRAM_POWER_INFO				0x61C

static pthread_mutex_t rapl_msr_lock = PTHREAD_MUTEX_INITIALIZER;
static int rapl_msr_instances=0;

#define RAPL_ENERGY_EV 2
#define RAPL_DRAM_EV 0
#define RAPL_PCK_EV 1



double power_units, cpu_energy_units, time_units, dram_energy_units;

int init_rapl_msr(int *fd_map)
{	
    int j;
    unsigned long long result;
	/* If it is not initialized, I do it, else, I get the ids */
		pthread_mutex_lock(&rapl_msr_lock);
    if (is_msr_initialized()==0){ 
			debug("MSR registers already initialized");
	    init_msr(fd_map);
    }else get_msr_ids(fd_map);
		rapl_msr_instances++;
	
    /* Ask for msr info */
    for(j=0;j<get_total_packages();j++) 
    {
        if (omsr_read(&fd_map[j], &result, sizeof result, MSR_INTEL_RAPL_POWER_UNIT)){ 
					debug("Error in omsr_read init_rapl_msr");
					pthread_mutex_unlock(&rapl_msr_lock);
					return EAR_ERROR;
				}

        power_units=pow(0.5,(double)(result&0xf));
        cpu_energy_units=pow(0.5,(double)((result>>8)&0x1f));
        time_units=pow(0.5,(double)((result>>16)&0xf));
        dram_energy_units=pow(0.5,(double)16);
    }
		pthread_mutex_unlock(&rapl_msr_lock);
    return EAR_SUCCESS;
}

/* DRAM 0, DRAM 1,..DRAM N, PCK0,PCK1,...PCKN */
int read_rapl_msr(int *fd_map,unsigned long long *_values)
{
	unsigned long long result;
	int j;
	int nump;
	nump=get_total_packages();

	for(j=0;j<nump;j++) {
		/* PKG reading */	    
	    if (omsr_read(&fd_map[j], &result, sizeof result, MSR_INTEL_PKG_ENERGY_STATUS)){
				debug("Error in omsr_read read_rapl_msr");
				return EAR_ERROR;
			}
		result &= 0xffffffff;
		_values[nump+j] = (unsigned long long)result*(cpu_energy_units*1000000000);

		/* DRAM reading */
	    if (omsr_read(&fd_map[j], &result, sizeof result, MSR_DRAM_ENERGY_STATUS)){
				debug("Error in omsr_read read_rapl_msr");
				return EAR_ERROR;
			}
		result &= 0xffffffff;

		_values[j] = (unsigned long long)result*(dram_energy_units*1000000000);
		
    }
	return EAR_SUCCESS;
}

void dispose_rapl_msr(int *fd_map)
{
	int j,tp;
	pthread_mutex_lock(&rapl_msr_lock);
  rapl_msr_instances--;
	if (rapl_msr_instances==0){
		tp=get_total_packages();
		for (j = 0; j < tp; j++) omsr_close(&fd_map[j]);
	}
	pthread_mutex_unlock(&rapl_msr_lock);
}

void diff_rapl_msr_energy(unsigned long long *diff,unsigned long long *end, unsigned long long *init)
{
  unsigned long long ret = 0;
  int nump,j;
  nump=get_total_packages();

	for(j=0;j<nump*RAPL_ENERGY_EV;j++) {
  	if (end[j] >= init[j]) {
    	ret = end[j] - init[j];
  	} else {
    	ret = ullong_diff_overflow(init[j], end[j]);
  	}
		diff[j]=ret;
	}
}

unsigned long long acum_rapl_energy(unsigned long long *values)
{
	unsigned long long ret = 0;
  int nump,j;
  nump=get_total_packages();
	for(j=0;j<nump*RAPL_ENERGY_EV;j++) {
		ret=ret+values[j];
	}	
	return ret;
}



void rapl_msr_energy_to_str(char *b,unsigned long long *values)
{
	int nump,j;
	char baux[512];
  nump=get_total_packages();

  sprintf(baux,", CPU (");
  for (j = 0; j < nump; j++) {
  	if (j < (nump - 1)) {
  		sprintf(b,"%llu,", values[nump*RAPL_PCK_EV+j]);
  	} else {
  		sprintf(b,"%llu)", values[nump*RAPL_PCK_EV+j]);
  	}
		strcat(baux,b);
  }

	sprintf(b,", DRAM (");
	strcat(baux,b);
	for (j = 0; j < nump; j++) {
		if (j < (nump - 1)) {
			sprintf(b,"%llu,", values[nump*RAPL_DRAM_EV+j]);
		} else {
			sprintf(b,"%llu)", values[nump*RAPL_DRAM_EV+j]);
		}
		strcat(baux,b);
	}
	strcpy(b,baux);

}