/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EAR_PC_APP_INFO_H
#define _EAR_PC_APP_INFO_H
#include <common/config.h>
#include <common/config/config_install.h>
#include <common/config/config_dev.h>
#include <common/types/types.h>
typedef struct pc_app_info{
	uint  powercap;
	uint  cpu_mode;
	ulong req_f[MAX_CPUS_SUPPORTED];
	ulong imc_f[MAX_SOCKETS_SUPPORTED];
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
