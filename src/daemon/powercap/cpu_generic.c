/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/
#define _GNU_SOURCE
// #define SHOW_DEBUGS 1

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>

#include <common/config.h>
#include <common/colors.h>
#include <common/states.h>
#include <common/system/monitor.h>
#include <common/output/verbose.h>
#include <common/hardware/hardware_info.h>

#include <management/cpupow/cpupow.h>
#include <management/cpufreq/cpufreq.h>

#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_status_conf.h>

static topology_t node_desc;
static uint current_limit = POWER_CAP_UNLIMITED;
static uint pc_on=0;
static uint num_pstates = 0;
static uint current_status = 0;
static float cpu_weight = 0;
static float dram_weight = 0;
static int32_t num_cpus = 0;
static int32_t num_dram = 0;
static uint32_t *cpu_power = NULL;
static uint32_t *dram_power = NULL;

static suscription_t *sus_cpu_pc;

static domain_settings_t settings = { .node_ratio = 0.1, .security_range = 0.01 }; //

state_t disable()
{
    mgt_cpupow_powercap_reset(CPUPOW_DRAM);
    return mgt_cpupow_powercap_reset(CPUPOW_SOCKET);
}

state_t enable(suscription_t *sus)
{
    state_t ret;

    ret = topology_init(&node_desc);
    if (ret == EAR_ERROR) {
        debug("Error getting node topology");
    }

    mgt_cpupow_load(&node_desc, 0);

    num_cpus = mgt_cpupow_count_devices(CPUPOW_SOCKET);
    num_dram = mgt_cpupow_count_devices(CPUPOW_DRAM);

    debug("num cpus: %u num_dram: %u", num_cpus, num_dram);

    uint32_t total_cpu_power  = 0;
    uint32_t total_dram_power = 0;

     cpu_power = calloc(num_cpus, sizeof(uint32_t));
    dram_power = calloc(num_dram, sizeof(uint32_t));

    mgt_cpupow_tdp_get(CPUPOW_SOCKET,  cpu_power);
    mgt_cpupow_tdp_get(CPUPOW_DRAM,   dram_power);

    for (int32_t i = 0; i < num_cpus; i++) {
        total_cpu_power += cpu_power[i];
    }
    for (int32_t i = 0; i < num_dram; i++) {
        total_dram_power += dram_power[i];
    }

    debug("total cpu tdp: %u total dram tdp: %u", total_cpu_power, total_dram_power);
     cpu_weight = (float)total_cpu_power  / (total_cpu_power + total_dram_power);
    dram_weight = (float)total_dram_power / (total_cpu_power + total_dram_power);
    debug("cpu weigt: %.2lf dram weight: %.2lf", cpu_weight, dram_weight);

    current_limit = UINT32_MAX;

    sus_cpu_pc = sus;
    return EAR_SUCCESS;
}

state_t plugin_set_burst()
{
    return monitor_burst(sus_cpu_pc, 0);
}

state_t plugin_set_relax()
{
    return monitor_relax(sus_cpu_pc);
}

void plugin_get_settings(domain_settings_t *s) 
{
    memcpy(s, &settings, sizeof(domain_settings_t));
}

static state_t _set_per_device_powercap_value(uint pid, uint32_t *limit, uint32_t *cpu_util)
{
    for (int32_t i = 0; i < num_cpus; i++) {
        if (limit[i] == POWER_CAP_UNLIMITED) {
            limit[i] = POWERCAP_DISABLE;
        }
        cpu_power[i] = limit[i];
        debug("power assigned to cpu%d: %u", i, cpu_power[i]);
    }
    state_t ret = EAR_ERROR;
    ret = mgt_cpupow_powercap_set(CPUPOW_SOCKET, cpu_power);
    if (ret == EAR_ERROR) {
        debug("error setting CPU powercap");
    }

    return ret;
}

static state_t _set_single_powercap_value(uint pid, uint32_t limit, uint32_t *cpu_util)
{
    debug("setting powercap %u", limit);
    if (limit == current_limit) return EAR_SUCCESS;

    current_limit = limit;
    if (limit == POWER_CAP_UNLIMITED) {
        mgt_cpupow_powercap_reset(CPUPOW_DRAM);
        return mgt_cpupow_powercap_reset(CPUPOW_SOCKET);
    }

    for (int32_t i = 0; i < num_cpus; i++) {
        cpu_power[i] = (limit * cpu_weight) / num_cpus;
        debug("power assigned to cpu%d: %u", i, cpu_power[i]);
    }
    for (int32_t i = 0; i < num_dram; i++) {
        dram_power[i] = (limit * dram_weight) / num_dram;
        debug("power assigned to dram%d: %u", i, dram_power[i]);
    }
    state_t ret = EAR_ERROR;
    ret = mgt_cpupow_powercap_set(CPUPOW_SOCKET, cpu_power);
    if (ret == EAR_ERROR) {
        debug("error setting CPU powercap");
        return ret;
    }
    ret = mgt_cpupow_powercap_set(CPUPOW_DRAM, dram_power);
    if (ret == EAR_ERROR) {
        debug("error setting DRAM powercap");
    }

    return ret;
}

state_t set_powercap_value(uint pid, uint32_t domain, uint32_t *limit, uint32_t *cpu_util)
{
    pc_on=1;

	if (domain == LEVEL_NODE || domain == LEVEL_DOMAIN) return _set_single_powercap_value(pid, *limit, cpu_util);
	else return _set_per_device_powercap_value(pid, limit, cpu_util);
}

state_t get_powercap_value(uint pid,uint *powercap)
{
    debug("CPU get_powercap_value");
    return mgt_cpupow_powercap_get(CPUPOW_SOCKET, powercap);
}

void set_status(uint status)
{
    current_status = status;
}

uint get_powercap_stragetgy()
{
    return PC_POWER;
}

void set_pc_mode(uint mode)
{
    debug("CPU powercap set_mode. pending");
}

#define MIN_CPU_POWER_MARGIN 10
/* Returns 0 when 1) No info, 2) No limit 3) We cannot share power. 
 *
 * state_t release_powercap_allocation(
 * Returns 1 when power can be shared with other modules 
 * Status can be: PC_STATUS_OK, PC_STATUS_GREEDY, PC_STATUS_RELEASE, PC_STATUS_ASK_DEF 
 * tbr is > 0 when PC_STATUS_RELEASE , 0 otherwise 
 * X_status is the plugin status 
 * X_ask_def means we must ask for our power  */

uint get_powercap_status(domain_status_t *status)
{
    if (!pc_on) return 0;
    /* Return 0 means we cannot release power */
    if (current_limit == POWER_CAP_UNLIMITED) return 0;


    return 0;
}

