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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
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

typedef struct app_id{
  ulong jid;
  ulong sid;
  ulong lid;
}app_id_t;


/* new_services : This section is eard request messages headers */
typedef struct eard_head_s {
  ulong    req_service;
  ulong    sec;
  size_t   size;
  state_t  state;
  app_id_t con_id;
} eard_head_t;

struct daemon_req {
/* For new_services : This section must include same size than eard_head_t */
  ulong    req_service;
  ulong    sec;
  size_t   size;
  state_t  state;
  app_id_t con_id;
  union daemon_req_opt req_data;
};

#define WAIT_DATA    0
#define NO_WAIT_DATA 1

int eards_read(int fd,char *buff,int size, uint type);
int eards_write(int fd,char *buff,int size);


/** Frequency readings and management */
#define START_GET_FREQ          1
#define END_GET_FREQ            2
#define CONNECT_FREQ            5
#define START_APP_COMP_FREQ     6
#define END_APP_COMP_FREQ       7

//
#define END_COMM                1000
//
#define EAR_COM_OK              0
#define EAR_COM_ERROR          -1

/* RAPL requests */
#define START_RAPL        200
#define RESET_RAPL        201
#define READ_RAPL         202
#define DATA_SIZE_RAPL      203

/* Report data */
#define WRITE_APP_SIGNATURE   300
#define WRITE_EVENT       302
#define WRITE_LOOP_SIGNATURE  303

/** Energy readings */
#define READ_DC_ENERGY      400
#define DATA_SIZE_ENERGY_NODE   401
#define ENERGY_FREQ       403


/** GPU management */
#define GPU_MODEL               500
#define GPU_DEV_COUNT           501
#define GPU_DATA_READ           502
#define GPU_SET_FREQ            503
#define GPU_GET_INFO            504
#define GPU_SUPPORTED           505


#define CONNECT_EARD_NODE_SERVICES 		CONNECT_FREQ
#define DISCONNECT_EARD_NODE_SERVICES END_COMM

/* Commands using new API will use service req > 1000 */
#define NEW_API_SERVICES        1000


#if 0
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
#define CONNECT_FREQ 			5
#define START_APP_COMP_FREQ 	6
#define END_APP_COMP_FREQ 		7

#define END_COMM 				1000
#define EAR_COM_OK     	 		0
#define EAR_COM_ERROR 			-1

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
#endif
