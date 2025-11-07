/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/config.h>
#include <common/config/config_install.h>
// #define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/types/pc_app_info.h>
#include <daemon/powercap/powercap_status_conf.h>

void pcapp_info_new_job(pc_app_info_t *t)
{
    memset(t->req_f, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);
    t->req_power = 0;
    t->pc_status = PC_STATUS_IDLE;
#if USE_GPUS
    t->num_gpus_used = 0;
    memset(t->req_gpu_f, 0, sizeof(ulong) * MAX_GPUS_SUPPORTED);
    t->req_gpu_power = 0;
#endif
}

void pcapp_info_end_job(pc_app_info_t *t)
{
    memset(t->req_f, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);
    t->req_power = 0;
    t->pc_status = PC_STATUS_IDLE;
#if USE_GPUS
    t->num_gpus_used = 0;
    memset(t->req_gpu_f, 0, sizeof(ulong) * MAX_GPUS_SUPPORTED);
    t->req_gpu_power = 0;
#endif
}

void pcapp_info_set_req_f(pc_app_info_t *t, ulong *f, uint num_cpus)
{
    memcpy(t->req_f, f, sizeof(ulong) * num_cpus);
    ;
    t->pc_status = PC_STATUS_OK;
}

void pcapp_info_set_gpu_req_f(pc_app_info_t *t, ulong *f, uint num_gpus)
{
#if USE_GPUS
    memcpy(t->req_gpu_f, f, num_gpus * sizeof(ulong));
#endif
    t->pc_status = PC_STATUS_OK;
}

void debug_pc_app_info(pc_app_info_t *t)
{
#if SHOW_DEBUGS
    int i;
    float f;
    f = (float) t->req_f[0] / 1000000.0;
    debug("PC_APP_INFO: mode %u CPU (req_f[0] %.2f power %lu status %u)", t->cpu_mode, f, t->req_power, t->pc_status);
#if USE_GPUS
    debug("GPU mode %u", t->gpu_mode);
    for (i = 0; i < t->num_gpus_used; i++) {
        f = (float) t->req_gpu_f[i] / 1000000.0;
        debug("\t(GPU[%d] req_f %.2f power %lu status %u)", i, f, t->req_gpu_power, t->pc_gpu_status);
    }
#endif
#endif
}
