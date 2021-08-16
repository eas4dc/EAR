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

#ifndef _APP_CONF_API_H
#define _APP_CONF_API_H
#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>

#include <common/config.h>

#define ENERGY_TIME		1000
#define ENERGY_TIME_DEBUG               1001
#define SELECT_CPU_FREQ									1002
#define SET_GPUFREQ											1003
#define SET_GPUFREQ_LIST								1004
#define CONNECT		2000
#define DISCONNECT 	2001
#define INVALID_COMMAND 1

typedef struct energy{
        ulong energy_j;
        ulong energy_mj;
        ulong time_sec;
        ulong time_ms;
        ulong os_time_sec;
        ulong os_time_ms;
}energy_t;

typedef union app_recv_opt {
	energy_t my_energy;
}app_recv_opt_t;

typedef struct cpu_freq_req{
	cpu_set_t mask;
	unsigned long cpuf;
}cpu_freq_req_t;

typedef struct gpu_freq_req{
	uint gpu_id;
	ulong gpu_freq;
}gpu_freq_req_t;

typedef struct gpu_freq_list_req{
	ulong gpu_freq[MAX_GPUS_SUPPORTED];
}gpu_freq_list_req_t;

typedef union app_send_data{
	cpu_freq_req_t cpu_freq;
	gpu_freq_req_t gpu_freq;
	gpu_freq_list_req_t gpu_freq_list;
}app_send_data_t;

typedef struct app_send{
	uint req;
	int pid;
	app_send_data_t	send_data;
}app_send_t;


typedef struct app_recv{
	int 				ret;
	app_recv_opt_t 	my_data;
}app_recv_t;

#else
#endif
