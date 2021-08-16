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

#ifndef _EAR_PC_APP_INFO_H
#define _EAR_PC_APP_INFO_H
#include <common/config.h>
#include <common/config/config_install.h>
#include <common/config/config_dev.h>
#include <common/types/types.h>
typedef struct pc_app_info{
	uint  cpu_mode;
	ulong req_f[MAX_CPUS_SUPPORTED];
	ulong imc_f[MAX_CPUS_SUPPORTED];
	ulong req_power;
	uint  pc_status;
	#if USE_GPUS
    uint  gpu_mode;
	ulong num_gpus_used;
	ulong req_gpu_f[MAX_GPUS_SUPPORTED];
	ulong req_gpu_power;
	uint  pc_gpu_status;
	#endif
}pc_app_info_t;

void pcapp_info_new_job(pc_app_info_t *t);
void pcapp_info_end_job(pc_app_info_t *t);

/** Sets the req_f and the status to OK */
void pcapp_info_set_req_f(pc_app_info_t *t, ulong *f, uint num_cpus);
/** Sets the gpu_req_f and the status to OK */
void pcapp_info_set_gpu_req_f(pc_app_info_t *t, ulong *f, uint num_gpus);
void debug_pc_app_info(pc_app_info_t *t);



#endif
