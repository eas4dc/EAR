/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <common/colors.h>
#include <common/config.h>
#include <common/states.h>
#include <pthread.h>
#include <signal.h>
// #define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/system/execute.h>
#include <common/system/monitor.h>
#include <daemon/powercap/powercap_status_conf.h>

static domain_settings_t settings = {.node_ratio = 0.00, .security_range = 0.00};

static uint current_gpu_pc = 0;
static uint gpu_pc_enabled = 0;
static uint c_status       = PC_STATUS_IDLE;
static uint c_mode         = PC_MODE_LIMIT;

#define NVIDIA_GPU_SET_POWER_LIMIT_CMD "nvidia-smi –pl %u"
#define NVIDIA_CPU_TEST_CMD            "nvidia-smi -h"

static uint gpu_dummy_num_gpus = 1;

state_t disable()
{
    return EAR_SUCCESS;
}

state_t enable(suscription_t *sus)
{
    debug("GPU_dummy: power cap  enable");
    gpu_pc_enabled = 1;
    return EAR_SUCCESS;
}

state_t plugin_set_relax()
{
    return EAR_SUCCESS;
}

state_t plugin_set_burst()
{
    return EAR_SUCCESS;
}

void plugin_get_settings(domain_settings_t *s)
{
    memcpy(s, &settings, sizeof(domain_settings_t));
}

state_t set_powercap_value(uint pid, uint domain, uint limit, uint *gpu_util)
{
    int i;
    /* Set data */
    debug("%sGPU_dummy: set_powercap_value %u%s", COL_BLU, limit, COL_CLR);
    current_gpu_pc = limit;
    for (i = 0; i < gpu_dummy_num_gpus; i++) {
        debug("GPU_dummy: util_gpu[%d]=%u", i, gpu_util[i]);
    }
    return EAR_SUCCESS;
}

state_t get_powercap_value(uint pid, uint *powercap)
{
    /* copy data */
    debug("GPU_dummy::get_powercap_value");
    *powercap = current_gpu_pc;
    return EAR_SUCCESS;
}

uint is_powercap_policy_enabled(uint pid)
{
    return gpu_pc_enabled;
}

void print_powercap_value(int fd)
{
    dprintf(fd, "GPU_dummy %u\n", current_gpu_pc);
}

void powercap_to_str(char *b)
{
    sprintf(b, "%u", current_gpu_pc);
}

void set_status(uint status)
{
    debug("GPU_dummy. set_status %u", status);
    c_status = status;
}

uint get_powercap_strategy()
{
    debug("GPU_dummy. get_powercap_strategy");
    return PC_POWER;
}

void set_pc_mode(uint mode)
{
    debug("GPU_dummy. set_pc_mode");
    c_mode = mode;
}

void set_verb_channel(int fd)
{
    WARN_SET_FD(fd);
    VERB_SET_FD(fd);
    DEBUG_SET_FD(fd);
}

void set_new_utilization(uint *util)
{
    int i;
    for (i = 0; i < gpu_dummy_num_gpus; i++) {
        debug("GPU_dummy: util_gpu[%d]=%u", i, util[i]);
    }
}
