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
#include <stdlib.h>
#include <string.h>
#include <common/config.h>
#include <common/states.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <daemon/node_metrics.h>
#include <metrics/energy/energy_cpu.h>
#include <metrics/temperature/temperature.h>

#define NM_CONNECTED	100

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

int init_node_metrics(nm_t *id, topology_t *topo, ulong def_freq)
{
	int sockets = topo->socket_count;
	int cpus_per_socket = topo->core_count;
	state_t s;

	if ((id==NULL)	|| (sockets<=0) || (cpus_per_socket <=0) || (def_freq<=0)){
		debug("init_node_metrics invalid argument id null=%d sockets=%u cpus_per_socket %u def_freq %lu\n",
			  (id==NULL),sockets,cpus_per_socket,def_freq);
		return EAR_ERROR;
	}

	//
	id->con=-1;
	id->ncpus=cpus_per_socket;
	id->nsockets=sockets;
	id->def_f=def_freq;

	// CPU Temperature
	init_temp_msr(nm_temp_fd);

	// CPU/IMC Frequency	
	state_assert(s, freq_cpu_init(topo),);
	state_assert(s, freq_imc_init(topo),);

	//
	id->con=NM_CONNECTED;

	return EAR_SUCCESS;
}
int init_node_metrics_data(nm_t *id,nm_data_t *nm)
{
	state_t s;

	if ((id==NULL) || (nm==NULL)){
		debug("init_node_metrics_data invalid argument\n");
		return EAR_ERROR;
	}

	// CPU Temperature
	nm->temp=(unsigned long long *)malloc(sizeof(uint64_t)*id->nsockets);

	if (nm->temp==NULL){
        	debug("init_node_metrics_data not enough memory\n");
        	return EAR_ERROR;
    	}

	// CPU/IMC Frequency
	state_assert(s, freq_cpu_data_alloc(&nm->freq_cpu, NULL, NULL),);
	state_assert(s, freq_imc_data_alloc(&nm->freq_imc, NULL, NULL),);
	nm->avg_cpu_freq=0;
	nm->avg_imc_freq=0;

	return EAR_SUCCESS;
}

int start_compute_node_metrics(nm_t *id,nm_data_t *nm)
{
	state_t s;

	if ((nm==NULL) || (id==NULL) || (id->con!=NM_CONNECTED)){
		debug("start_compute_node_metrics invalid argument");
		return EAR_ERROR;
	}

	// CPU/IMC Frequency
	state_assert(s, freq_cpu_read(&nm->freq_cpu),);
	state_assert(s, freq_imc_read(&nm->freq_imc),);

	return EAR_SUCCESS;
}

int end_compute_node_metrics(nm_t *id,nm_data_t *nm)
{
	state_t s;
	int i;

	if ((nm==NULL)|| (id==NULL) || (id->con!=NM_CONNECTED)){
		debug("end_compute_node_metrics invalid argument");
		return EAR_ERROR;
	}

	// CPU Temperature
	for (i=0;i<id->nsockets;i++) nm->temp[i]=0;
	if (read_temp_msr(nm_temp_fd,nm->temp)!=EAR_SUCCESS) return EAR_ERROR;

	// CPU/IMC Frequency
	state_assert(s, freq_cpu_read(&nm->freq_cpu),);
	state_assert(s, freq_imc_read(&nm->freq_imc),);

	return EAR_SUCCESS;
}

int diff_node_metrics(nm_t *id,nm_data_t *init,nm_data_t *end,nm_data_t *diff_nm)
{
	state_t s;
	int i;

	if ((init==NULL) || (end==NULL) || (diff_nm==NULL)){
		debug("diff_node_metrics invalid argument");
		return EAR_ERROR;
	}

	// ????????????
	// memcpy(diff_nm, end, sizeof(nm_data_t));
	
	for (i=0;i<id->nsockets;i++){
		diff_nm->temp[i]=end->temp[i];
	}

	// CPU & IMC Frequency
	state_assert(s, freq_cpu_data_diff(&end->freq_cpu, &init->freq_cpu, NULL, &diff_nm->avg_cpu_freq),);
	state_assert(s, freq_imc_data_diff(&end->freq_imc, &init->freq_imc, NULL, &diff_nm->avg_imc_freq),);

	return EAR_SUCCESS;
}

int dispose_node_metrics(nm_t *id)
{
	state_t s;

	if ((id==NULL) || (id->con!=NM_CONNECTED)){
		debug("dispose_node_metrics invalid id");
		return EAR_ERROR;
	}

	// Alomejor podemos implementar un sistema de contadores aquí para poder
	// cerrar también el de la CPU?
	state_assert(s, freq_imc_dispose(),);

	return EAR_SUCCESS;
}

int copy_node_metrics(nm_t *id, nm_data_t *dest, nm_data_t *src)
{
	state_t s;
	int i;

	if ((dest==NULL) || (src==NULL)){
		debug("copy_node_metrics invalid argument");
		return EAR_ERROR;
	}

	// ??????????
	// memcpy(dest,src,sizeof(nm_data_t));

	for (i=0;i<id->nsockets;i++){
		dest->temp[i]=src->temp[i];
	}

	state_assert(s, freq_cpu_data_copy(&dest->freq_cpu, &src->freq_cpu),);
	state_assert(s, freq_imc_data_copy(&dest->freq_imc, &src->freq_imc),);
	dest->avg_cpu_freq = src->avg_cpu_freq;
	dest->avg_imc_freq = src->avg_imc_freq;

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
	printf("node_metrics avg_imc_freq %lu ",nm->avg_imc_freq);
	for (i=0;i<id->nsockets;i++) printf("temp[%d]=%llu ",i,nm->temp[i]);

	return EAR_SUCCESS;
}

int verbose_node_metrics(nm_t *id,nm_data_t *nm)
{
	ullong temp_total=0;
	char msg[1024];
	int i;

	if ((nm==NULL) || (id==NULL) || (id->con!=NM_CONNECTED)){
		debug("verbose_node_metrics invalid argument");
		return EAR_ERROR;
	}

	for (i=0;i<id->nsockets;i++){
		temp_total+=nm->temp[i];
	}
	temp_total=temp_total/id->nsockets;

	sprintf(msg," avg_cpu_freq=%.2lf, avg_imc_freq=%.2lf, temp=%llu",
		(double)nm->avg_cpu_freq/(double)1000000,
		(double)nm->avg_imc_freq/(double)1000000,
		temp_total);

	verbose(VNODEPMON,msg);
	return EAR_SUCCESS;
}
