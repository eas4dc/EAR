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

#ifndef _EAR_DAEMON_COMMON_H
#define _EAR_DAEMON_COMMON_H

#define _GNU_SOURCE             
#include <sched.h>

#include <common/config/config_install.h>
#include <common/types/generic.h>
#include <common/types/application.h>
#include <common/types/job.h>
#include <common/types/loop.h>
#include <common/types/event_type.h>

typedef struct gpu_freq_req{
	uint  num_dev;
	ulong gpu_freqs[MAX_GPUS_SUPPORTED];
}gpu_freq_req_t;

typedef struct cpu_freq_req{
  uint  num_cpus;
  ulong cpu_freqs[MAX_CPUS_SUPPORTED];
}cpu_freq_req_t;

typedef struct new_freq_type{
	unsigned long f;
	cpu_set_t mask;	
}new_freq_type_t;

typedef struct unc_freq_req{
	// Max and min for each socket 
	uint index_list[MAX_SOCKETS_SUPPORTED*2];	
}unc_freq_req_t;

// Data type to send the requests
union daemon_req_opt {
    unsigned long req_value;
    application_t app;
	loop_t		  loop;
	ear_event_t event;
	new_freq_type_t f_mask;
	gpu_freq_req_t  gpu_freq;
	cpu_freq_req_t  cpu_freq;
	unc_freq_req_t	unc_freq;
};

struct daemon_req {
    ulong req_service;
	ulong  sec;
    union daemon_req_opt req_data;
};

// Number of services supported
#define ear_daemon_client_requests 	1
#define freq_req 					0
#define uncore_req 					0
#define rapl_req 					0
#define system_req 					0
#define node_energy_req 			0
#define gpu_req 			0

// Services related with frequency
#define SET_FREQ 				0
#define START_GET_FREQ 			1
#define END_GET_FREQ 			2
#define SET_TURBO 				3
#define DATA_SIZE_FREQ 			4
#define CONNECT_FREQ 			5
#define START_APP_COMP_FREQ 	6
#define END_APP_COMP_FREQ 		7
#define SET_FREQ_WITH_MASK 8
#define SET_NODE_FREQ_WITH_LIST 9
#define GET_CPUFREQ				10
#define GET_CPUFREQ_LIST	11

/*** NEW FREQ functions */
#define READ_CPUFREQ			12

/* UNC frequency management */
#define UNC_SIZE                20
#define UNC_READ                21
#define UNC_GET_LIMITS          22
#define UNC_SET_LIMITS          23
#define UNC_INIT_DATA           24
#define UNC_GET_LIMITS_MIN      25
#define UNC_SET_LIMITS_MIN      26

#define END_COMM 				1000
#define EAR_COM_OK     	 		0
#define EAR_COM_ERROR 			-1

// Services related with uncore counters
#define START_UNCORE 			100
#define RESET_UNCORE 			101
#define READ_UNCORE 			102
#define DATA_SIZE_UNCORE 		103
#define CONNECT_UNCORE 			104
#define STOP_UNCORE 			105

// Services related with rapl counters 
#define START_RAPL 				200
#define RESET_RAPL 				201
#define READ_RAPL 				202
#define DATA_SIZE_RAPL 			203
#define CONNECT_RAPL 			204

// Services to contact with the rest of the system, such as writting data
#define WRITE_APP_SIGNATURE 	300
#define CONNECT_SYSTEM 			301
#define WRITE_EVENT				302
#define WRITE_LOOP_SIGNATURE 	303

// Services related to node energy management
#define READ_DC_ENERGY 			400
#define DATA_SIZE_ENERGY_NODE 	401
#define CONNECT_ENERGY 			402
#define ENERGY_FREQ				403

#define GPU_MODEL               500
#define GPU_DEV_COUNT           501
#define GPU_DATA_READ           502
#define GPU_SET_FREQ            503
#define GPU_GET_INFO            504

#endif
