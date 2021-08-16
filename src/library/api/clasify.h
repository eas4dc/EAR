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

#ifndef APP_CLASSIFICATION
#define APP_CLASSIFICATION
#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/types/signature.h>


#define APP_COMP_BOUND 	1
#define APP_MPI_BOUND 	2
#define APP_IO_BOUND		3
#define APP_BUSY_WAITING	4

#define GPU_IDLE 1
#define GPU_COMP 0

#define APP_CPU_BOUND APP_COMP_BOUND


#define DYNAIS_GUIDED 0
#define TIME_GUIDED 	1

#define USEFUL_COMP 0
#define BUSY_WAITING 1
// #define CPI_CPU_BOUND 0.4
#define CPI_CPU_BOUND 			0.6
#define GBS_CPU_BOUND 			30.0
#define CPI_BUSY_WAITING		0.6
#define GBS_BUSY_WAITING 		3.0
#define GFLOPS_BUSY_WAITING 0.1
#define IO_TH   						10.0
#define MPI_TH 							10000
#define MIN_MPI_TH 					200
#define CPI_MEM_BOUND 			1.0
#define GBS_MEM_BOUND				60.0

//#define FLOPS_CPU_BOUND 100.0

//state_t classify(ullong total_rw,uint total_mpi, float rwsec,float mpisec,uint dyn_on,uint dynais_state,uint periodic_mode,uint earl_state,uint *app_state,uint *ear_guided);
state_t classify(uint iobound,uint mibound, uint *app_state);
state_t is_cpu_busy_waiting(signature_t *sig,uint *busy);
state_t is_cpu_bound(signature_t *sig,uint *cbound);
state_t is_mem_bound(signature_t *sig,uint *mbound);
state_t is_gpu_idle(signature_t *sig,uint *gidle);
state_t is_io_bound(float rwsec,uint *iobound);
state_t is_network_bound(float mpisec,uint *mpibound);

state_t must_switch_to_time_guide(float mpisec, uint *ear_guided);



#endif
