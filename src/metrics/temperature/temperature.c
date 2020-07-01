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

#include <stdlib.h>

//#define SHOW_DEBUGS 0

#include <common/config.h>
#include <metrics/common/msr.h>
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
        if (msr_read(fd_map, &result, sizeof result, MSR_TEMPERATURE_TARGET)) return EAR_ERROR;
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
        if (msr_read(&fds[j], &result, sizeof result, IA32_PKG_THERM_STATUS)) return EAR_ERROR;
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
        if (msr_read(&fds[j], &result, sizeof result, IA32_PKG_THERM_STATUS)) return EAR_ERROR;
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
        if (msr_read(&fds[j], &result, sizeof result, IA32_PKG_THERM_STATUS)) return EAR_ERROR;
        result &= ~(0x2);
        if (msr_write(&fds[j], &result, sizeof result, IA32_PKG_THERM_STATUS)) return EAR_ERROR;

    }
	return EAR_SUCCESS;
}
