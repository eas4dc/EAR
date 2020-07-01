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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/config.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/states.h>
#include <daemon/node_metrics.h>
#include <metrics/frequency/frequency_cpu.h>
#include <metrics/frequency/frequency_imc.h>
#include <metrics/energy/energy_cpu.h>
#include <metrics/temperature/temperature.h>

#define NM_CONNECTED	100

/*
typedef struct node_metrics{
ulong avg_cpu_freq;
uint64_t *uncore_freq;
uint64_t *temp;
}nm_data_t;
*/

static int nm_temp_fd[MAX_PACKAGES];

unsigned long long get_nm_temp(nm_t *id,nm_data_t *nm)
{
	int i;
	unsigned long long temp_total=0;
	if ((id==NULL) || (nm==NULL) || (id->con!=NM_CONNECTED) || (nm->temp==NULL))	return 0;
	temp_total=nm->temp[0];
	for (i=1;i<id->nsockets;i++){
        if (temp_total<nm->temp[i]) temp_total=nm->temp[i];
    }
	return temp_total;
}

ulong get_nm_cpufreq(nm_t *id,nm_data_t *nm)
{
	if ((id==NULL) || (nm==NULL) || (id->con!=NM_CONNECTED))  return 0;
	return nm->avg_cpu_freq;
}

int init_node_metrics(nm_t *id,int sockets, int cpus_per_socket,int cores_model,ulong def_freq)
{
	if ((id==NULL)	|| (sockets<=0) || (cpus_per_socket <=0) || (def_freq<=0)){
		debug("init_node_metrics invalid argument id null=%d sockets=%u cpus_per_socket %u def_freq %lu\n",(id==NULL),sockets,cpus_per_socket,def_freq);
		return EAR_ERROR;
	}
	id->con=-1;
	id->ncpus=cpus_per_socket;
	id->nsockets=sockets;
	id->def_f=def_freq;
	if (frequency_uncore_init(sockets,cpus_per_socket,cores_model)!=EAR_SUCCESS){ 
		debug("Error when initializing frequency_uncore_init");
		return EAR_ERROR;
	}
	aperf_periodic_avg_frequency_init_all_cpus();
	init_temp_msr(nm_temp_fd);
	id->con=NM_CONNECTED;
	return EAR_SUCCESS;
}
int init_node_metrics_data(nm_t *id,nm_data_t *nm)
{
	if ((id==NULL) || (nm==NULL)){
		debug("init_node_metrics_data invalid argument\n");
		return EAR_ERROR;
	}
	nm->avg_cpu_freq=0;
	nm->uncore_freq=(uint64_t *)malloc(sizeof(uint64_t)*id->nsockets);
	if (nm->uncore_freq==NULL){
		debug("init_node_metrics_data not enough memory\n");
		return EAR_ERROR;
	}
	nm->temp=(unsigned long long *)malloc(sizeof(uint64_t)*id->nsockets);
    if (nm->temp==NULL){
        debug("init_node_metrics_data not enough memory\n");
        return EAR_ERROR;
    }
	return EAR_SUCCESS;
}


int start_compute_node_metrics(nm_t *id,nm_data_t *nm)
{
	if ((nm==NULL) || (id==NULL) || (id->con!=NM_CONNECTED)){
		debug("start_compute_node_metrics invalid argument");
		return EAR_ERROR;
	}
	if (frequency_uncore_counters_start()!=EAR_SUCCESS) return EAR_ERROR;
	return EAR_SUCCESS;
}

int end_compute_node_metrics(nm_t *id,nm_data_t *nm)
{
	int i;
	if ((nm==NULL)|| (id==NULL) || (id->con!=NM_CONNECTED)){
		debug("end_compute_node_metrics invalid argument");
		return EAR_ERROR;
	}
	nm->avg_cpu_freq=0;
	for (i=0;i<id->nsockets;i++) nm->uncore_freq[i]=0;
	for (i=0;i<id->nsockets;i++) nm->temp[i]=0;
	nm->avg_cpu_freq=aperf_periodic_avg_frequency_end_all_cpus();
	if (frequency_uncore_counters_stop(nm->uncore_freq)!=EAR_SUCCESS) return EAR_ERROR;
	if (read_temp_msr(nm_temp_fd,nm->temp)!=EAR_SUCCESS) return EAR_ERROR;
	return EAR_SUCCESS;
}

int diff_node_metrics(nm_t *id,nm_data_t *init,nm_data_t *end,nm_data_t *diff_nm)
{
	if ((init==NULL) || (end==NULL) || (diff_nm==NULL)){
		debug("diff_node_metrics invalid argument");
		return EAR_ERROR;
	}
	memcpy(diff_nm,end,sizeof(nm_data_t));
	return EAR_SUCCESS;
}

int dispose_node_metrics(nm_t *id)
{
	if ((id==NULL) || (id->con!=NM_CONNECTED)){
		debug("dispose_node_metrics invalid id");
		return EAR_ERROR;
	}
	if (frequency_uncore_dispose()!=EAR_SUCCESS){
		debug("dispose_node_metrics frequency_uncore_dispose error");
		return EAR_ERROR;
	}
	return EAR_SUCCESS;
}

int copy_node_metrics(nm_t *id,nm_data_t* dest, nm_data_t * src)
{
	if ((dest==NULL) || (src==NULL)){
		debug("copy_node_metrics invalid argument");
		return EAR_ERROR;
	}
	memcpy(dest,src,sizeof(nm_data_t));
	return EAR_SUCCESS;
}

int print_node_metrics(nm_t *id,nm_data_t *nm)
{
	int i;
	if ((id==NULL) || (nm==NULL) || (id->con!=NM_CONNECTED)){
		debug("print_node_metrics invalid argument");
		return EAR_ERROR;
	}
	printf("node_metrics avg_cpu_freq %lu ",nm->avg_cpu_freq);
	for (i=0;i<id->nsockets;i++) printf("uncore_freq[%d]=%llu ",i,(long long unsigned int)nm->uncore_freq[i]);
	for (i=0;i<id->nsockets;i++) printf("temp[%d]=%llu ",i,nm->temp[i]);
	return EAR_SUCCESS;
}

int verbose_node_metrics(nm_t *id,nm_data_t *nm)
{
	int i;
	char msg[1024];
	uint64_t uncore_total=0;
	unsigned long long temp_total=0;
	if ((nm==NULL) || (id==NULL) || (id->con!=NM_CONNECTED)){
		debug("verbose_node_metrics invalid argument");
		return EAR_ERROR;
	}
	for (i=0;i<id->nsockets;i++){
		uncore_total+=nm->uncore_freq[i];
	}
	uncore_total=uncore_total/2600000000;
	for (i=0;i<id->nsockets;i++){
		temp_total+=nm->temp[i];
	}
	temp_total=temp_total/id->nsockets;
	sprintf(msg," avg_cpu_freq %.2lf uncore_freq=%llu temp=%llu",(double)nm->avg_cpu_freq/(double)1000000,(long long unsigned int)uncore_total,temp_total);
	verbose(VNODEPMON,msg);
	return EAR_SUCCESS;
}
