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

#include <stdlib.h>

//#define SHOW_DEBUGS 0

#include <common/config.h>
#include <metrics/common/omsr.h>
#include <common/output/verbose.h>
/* Thermal Domain */
#define IA32_THERM_STATUS               0x19C
#define IA32_PKG_THERM_STATUS           0x1B1
#define MSR_TEMPERATURE_TARGET          0x1A2
int *throttling_temp;

int init_temp_msr(int *fd_map)
{
    int j;
    unsigned long long result;
    if (is_msr_initialized()==0){
        debug("Temperature: msr was not initialized, initializing");
        init_msr(fd_map);
    }else get_msr_ids(fd_map);

    throttling_temp = calloc(get_total_packages(), sizeof(int));
    for(j=0;j<get_total_packages();j++) {
        if (omsr_read(fd_map, &result, sizeof result, MSR_TEMPERATURE_TARGET)) return EAR_ERROR;
            throttling_temp[j] = (result >> 16);
    }

    return EAR_SUCCESS;
}

int read_temp_msr(int *fds,unsigned long long *_values)
{
	unsigned long long result;
	int j;

	for(j=0;j<get_total_packages();j++)
	{
	    /* PKG reading */    
        if (omsr_read(&fds[j], &result, sizeof result, IA32_PKG_THERM_STATUS)) return EAR_ERROR;
	    _values[j] = throttling_temp[j] - ((result>>16)&0xff);

    }
	return EAR_SUCCESS;
}

int read_temp_limit_msr(int *fds, unsigned long long *_values)
{
	unsigned long long result;
	int j;

	for(j=0;j<get_total_packages();j++)
	{
	    /* PKG reading */    
        if (omsr_read(&fds[j], &result, sizeof result, IA32_PKG_THERM_STATUS)) return EAR_ERROR;
	    _values[j] = ((result)&0x2);

    }
	return EAR_SUCCESS;
}

int reset_temp_limit_msr(int *fds)
{
	unsigned long long result;
	int j;

	for(j=0;j<get_total_packages();j++)
	{
	    /* PKG reading */    
        if (omsr_read(&fds[j], &result, sizeof result, IA32_PKG_THERM_STATUS)) return EAR_ERROR;
        result &= ~(0x2);
        if (omsr_write(&fds[j], &result, sizeof result, IA32_PKG_THERM_STATUS)) return EAR_ERROR;

    }
	return EAR_SUCCESS;
}
