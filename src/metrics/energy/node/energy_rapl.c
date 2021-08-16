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

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <common/states.h>
#include <common/math_operations.h>
#include <common/hardware/hardware_info.h>
#include <metrics/energy/cpu.h>
#include <metrics/energy/node/energy_node.h>

//#define SHOW_DEBUGS 1
#ifdef SHOW_DEBUGS
#define plug_debug(...) \
{ \
fprintf(stderr,__VA_ARGS__); \
fprintf(stderr,"\n"); \
}

#else
#define plug_debug(...)
#endif


static pthread_mutex_t rapl_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * MAIN FUNCTIONS
 */
static int num_pack=-1;

void check_num_packs()
{
	if (num_pack<0){
		num_pack=detect_packages(NULL);
	}
}
state_t energy_init(void **c)
{
	int *pfd,nfd;
	plug_debug("energy_rapl");
	if (c == NULL) return EAR_ERROR;
	num_pack = detect_packages(NULL);
	if (num_pack == 0){ 
		plug_debug("No packages detected in energy_init");
		return EAR_ERROR;
	}
	plug_debug("%d packages detected ",num_pack);
	*c = (int  *)malloc(sizeof(int)*num_pack);
	if (*c == NULL) return EAR_ERROR;
	pthread_mutex_lock(&rapl_lock);
	pfd = *c;
	nfd = init_rapl_msr(pfd);
	pthread_mutex_unlock(&rapl_lock);
	if (nfd<0){ 
					plug_debug("init_rapl_msr returns error in energy_init");
					return EAR_ERROR;
	}
	plug_debug("init_rapl_msr in energy_init successfully initialized in enery_rapl");
	return EAR_SUCCESS;
}
state_t energy_dispose(void **c)
{
				int *pfd;
				if ((c == NULL) || (*c == NULL)) return EAR_ERROR;
				pthread_mutex_lock(&rapl_lock);
				pfd = *c;
				dispose_rapl_msr(pfd);	
				free(*c);
				pthread_mutex_unlock(&rapl_lock);
				return EAR_SUCCESS;
}
state_t energy_datasize(size_t *size)
{
				check_num_packs();
				plug_debug("size %lu",sizeof(unsigned long long)*RAPL_POWER_EVS*num_pack);
				*size = sizeof(unsigned long long)*RAPL_POWER_EVS*num_pack;
				return EAR_SUCCESS;
}
state_t energy_frequency(ulong *freq_us)
{
				plug_debug("freq %d",10000);
				*freq_us = 10000;	
				return EAR_SUCCESS;
}

state_t energy_dc_read(void *c, edata_t energy_mj)
{
				int *pfd;
				pfd = (int *)c;

				unsigned long long *pvalues = energy_mj;
				plug_debug("setting values to null and calling read_rapl");
				memset(pvalues,0,sizeof(unsigned long long)*RAPL_POWER_EVS*num_pack);
				return read_rapl_msr(pfd,pvalues);
}


state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms)
{
				struct timeval t;
				int *pfd;
				pfd = (int *)c;

				unsigned long long *pvalues = energy_mj;
				memset(pvalues,0,sizeof(unsigned long long)*RAPL_POWER_EVS*num_pack);
				read_rapl_msr(pfd,pvalues);
				*time_ms = 0;
				gettimeofday(&t, NULL);
				*time_ms = t.tv_sec*1000 + t.tv_usec/1000;
				return EAR_SUCCESS;
}
state_t energy_ac_read(void *c, edata_t energy_mj)
{

				return EAR_SUCCESS;
}

unsigned long long diff_RAPL_energy(unsigned long long init,unsigned long long end)
{
				unsigned long long ret = 0;

				if (end >= init){
								ret = end - init;
				}else{
								ret = ullong_diff_overflow(init, end);
				}
				return ret;
}

state_t energy_units(uint *units)
{
				*units = 1000000000;
				return EAR_SUCCESS;
}
state_t energy_accumulated(unsigned long *e,edata_t init,edata_t end)
{

				int i;
				unsigned long total = 0;
				unsigned long long diff;
				unsigned long long *pvalues_init,*pvalues_end;
				pvalues_init = (unsigned long long *)init;
				pvalues_end  = (unsigned long long *)end;
				check_num_packs();
				for (i=0; i < RAPL_POWER_EVS*num_pack; i++){
								diff   = diff_RAPL_energy(pvalues_init[i],pvalues_end[i]);
								total += diff;	
				}
				plug_debug("energy_accumulated in plugin %lu",total);
				*e = total;
				return EAR_SUCCESS;
}


state_t energy_to_str(char *str,edata_t e)
{
				ullong *pe = (ullong *)e;
				int j;
				char msg[4096];
				plug_debug("energy_to_str energy_rapl");
				check_num_packs();
				sprintf(str,"DRAM-PLUGIN (");
				for (j = 0; j < num_pack; j++) {
								if (j < (num_pack - 1)) {
												sprintf(msg,"%llu,", pe[j]);
								} else {
												sprintf(msg,"%llu)", pe[j]);
								}
								strcat(str,msg);
				}
				strcat(str,", CPU-PLUGIN (");
				for (j = 0; j < num_pack; j++) {
								if (j < (num_pack - 1)) {
												sprintf(msg,"%llu,", pe[j]);
								} else {
												sprintf(msg,"%llu)", pe[j]);
								}
								strcat(str,msg);
				}
				return EAR_SUCCESS;
}
uint energy_data_is_null(edata_t e)  
{
				unsigned long long *pe = (unsigned long long *)e;
				int i;
				unsigned long long total = 0;
				check_num_packs();
				for (i=0;i< RAPL_POWER_EVS*num_pack; i++){ total += pe[i];}
				return (total == 0);
}

