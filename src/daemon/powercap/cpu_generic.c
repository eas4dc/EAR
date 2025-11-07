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
#define SHOW_DEBUGS 1

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/colors.h>
#include <common/config.h>
#include <common/hardware/hardware_info.h>
#include <common/output/verbose.h>
#include <common/states.h>
#include <common/system/monitor.h>
#include <common/system/time.h>

#include <metrics/energy_cpu/energy_cpu.h>

#include <management/cpufreq/cpufreq.h>
#include <management/cpupow/cpupow.h>

#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_status_conf.h>

static topology_t node_desc;
static uint current_limit     = POWER_CAP_UNLIMITED;
static uint pc_on             = 0;
static uint num_pstates       = 0;
static uint current_status    = 0;
static float cpu_weight       = 0;
static float dram_weight      = 0;
static int32_t num_cpus       = 0;
static int32_t num_dram       = 0;
static int32_t node_size      = 0;
static uint32_t energy_packs  = 0;
static uint32_t *cpu_power    = NULL;
static uint32_t *dram_power   = NULL;
static uint64_t *c_req_f      = NULL;
static uint64_t *energy_ct1   = NULL;
static uint64_t *energy_ct2   = NULL;
static uint64_t *energy_diff  = NULL;
static pstate_t *av_pstates   = NULL;
static pstate_t *curr_pstates = NULL;
static timestamp last_time    = {0};
static timestamp curr_time    = {0};

static suscription_t *sus_cpu_pc;

static domain_settings_t settings = {.node_ratio = 0.1, .security_range = 0.01}; //

state_t disable()
{
    mgt_cpupow_powercap_reset(CPUPOW_DRAM, RESET_DEFAULT);
    return mgt_cpupow_powercap_reset(CPUPOW_SOCKET, RESET_DEFAULT);
}

state_t enable(suscription_t *sus)
{
    state_t ret;

    ret = topology_init(&node_desc);
    if (ret == EAR_ERROR) {
        debug("Error getting node topology");
    }

    energy_cpu_load(&node_desc, 0);
    energy_cpu_init(NULL);
    mgt_cpupow_load(&node_desc, 0);
    mgt_cpufreq_load(&node_desc, 0);
    mgt_cpufreq_init(NULL);

    debug("num cpus: %u num_dram: %u", num_cpus, num_dram);

    /* cpufreq init */
    mgt_cpufreq_data_alloc(&av_pstates, NULL);
    mgt_cpufreq_data_alloc(&curr_pstates, NULL);
    mgt_cpufreq_get_available_list(NULL, (const pstate_t **) &av_pstates, &num_pstates);
    c_req_f   = calloc(MAX_CPUS_SUPPORTED, sizeof(uint64_t));
    node_size = node_desc.cpu_count;

    /* energy_cpu init */
    energy_cpu_data_alloc(NULL, (ullong **) &energy_ct1, &energy_packs);
    energy_cpu_data_alloc(NULL, (ullong **) &energy_ct2, NULL);
    energy_cpu_data_alloc(NULL, (ullong **) &energy_diff, NULL);
    energy_cpu_read(NULL, (ullong *) energy_ct1);

    debug("Energy monitoring initialized: %u packs", energy_packs);
    for (int i = 0; i < energy_packs; i++) {
        debug("Initial energy_ct1[%d] = %lu", i, energy_ct1[i]);
    }

    /* Get TDP and calculate CPU/DRAM weight*/
    uint32_t total_cpu_power  = 0;
    uint32_t total_dram_power = 0;

    num_cpus = mgt_cpupow_count_devices(CPUPOW_SOCKET);
    num_dram = mgt_cpupow_count_devices(CPUPOW_DRAM);

    debug("num cpus: %u num_dram: %u", num_cpus, num_dram);

    cpu_power  = calloc(num_cpus, sizeof(uint32_t));
    dram_power = calloc(num_dram, sizeof(uint32_t));

    mgt_cpupow_tdp_get(CPUPOW_SOCKET, cpu_power);
    mgt_cpupow_tdp_get(CPUPOW_DRAM, dram_power);

    for (int32_t i = 0; i < num_cpus; i++) {
        total_cpu_power += cpu_power[i];
    }
    for (int32_t i = 0; i < num_dram; i++) {
        total_dram_power += dram_power[i];
    }

    debug("total cpu tdp: %u total dram tdp: %u", total_cpu_power, total_dram_power);
    cpu_weight  = (float) total_cpu_power / (total_cpu_power + total_dram_power);
    dram_weight = (float) total_dram_power / (total_cpu_power + total_dram_power);
    debug("cpu weigt: %.2lf dram weight: %.2lf", cpu_weight, dram_weight);

    current_limit = POWER_CAP_UNLIMITED;
    sus_cpu_pc    = sus;

    timestamp_getfast(&last_time);

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
    ret         = mgt_cpupow_powercap_set(CPUPOW_SOCKET, cpu_power);
    if (ret == EAR_ERROR) {
        debug("error setting CPU powercap");
    }

    return ret;
}

static state_t _set_single_powercap_value(uint pid, uint32_t limit, uint32_t *cpu_util)
{
    debug("setting powercap %u", limit);
    if (limit == current_limit)
        return EAR_SUCCESS;

    if (limit == POWER_CAP_UNLIMITED) {
        mgt_cpupow_powercap_reset(CPUPOW_DRAM, RESET_DEFAULT);
        return mgt_cpupow_powercap_reset(CPUPOW_SOCKET, RESET_DEFAULT);
    }

    for (int32_t i = 0; i < num_cpus; i++) {
        cpu_power[i] = (limit * cpu_weight) / num_cpus;
        debug("power assigned to cpu%d: %u", i, cpu_power[i]);
    }
    for (int32_t i = 0; i < num_dram; i++) {
        dram_power[i] = (limit * dram_weight) / num_dram;
        debug("power assigned to dram%d: %u", i, dram_power[i]);
    }

    state_t ret = mgt_cpupow_powercap_set(CPUPOW_SOCKET, cpu_power);
    if (ret == EAR_ERROR) {
        debug("error setting CPU powercap");
        return ret;
    }
    current_limit = limit; // TODO: not exact, we don't know if dram will succeed.
    ret           = mgt_cpupow_powercap_set(CPUPOW_DRAM, dram_power);
    if (ret == EAR_ERROR) {
        debug("error setting DRAM powercap");
    }

    return ret;
}

state_t set_powercap_value(uint pid, uint32_t domain, uint32_t *limit, uint32_t *cpu_util)
{
    pc_on = 1;

    if (domain == LEVEL_DEVICE)
        return _set_per_device_powercap_value(pid, limit, cpu_util);
    else
        return _set_single_powercap_value(pid, *limit, cpu_util);
}

state_t get_powercap_value(uint pid, uint *powercap)
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

void set_app_req_freq(ulong *f)
{
    debug("DVFS:Requested application freq[0] set to %lu", f[0]);
    memcpy(c_req_f, f, sizeof(ulong) * MAX_CPUS_SUPPORTED);
}

/* Returns true if some pstate in p1 is higher than p2 */
static int _cmp_curr_pstate_with_target_freq(pstate_t *curr_pstate, uint64_t *target)
{
    for (int32_t i = 0; i < MAX_CPUS_SUPPORTED && i < num_cpus; i++) {
        uint32_t t_pstate = 0;
        mgt_cpufreq_get_index(NULL, target[i], &t_pstate, true);
        if (t_pstate > curr_pstate[i].idx)
            return 1;
    }
    return 0;
}

bool _pstates_are_equal(pstate_t *p1, uint64_t *p2)
{
    for (int32_t i = 0; i < MAX_CPUS_SUPPORTED; i++) {
        if (p1[i].idx != p2[i])
            return false;
    }

    return true;
}

/* Computes how many cpus can reduce one pstate with the given limit */
void num_cpus_to_reduce_pstates(uint *tmp, uint64_t *target, uint *changes, uint *changes_max)
{
    int i, logical_pstates;
    *changes     = 0;
    *changes_max = 0;
    ulong tmp_freq, next_freq;

    /* A logical pstate is a diff of 100000 MHz and is an average increase of 6 Watts
     * (except the first pstate). In some cases there is a jump in available frequencies
     * for a CPU of 2 logical p_states (and only 1 "real") so we use the logical steps
     * to compute the necessary power. */
    for (i = 0; i < node_size; i++) {
        if (tmp[i] == 0)
            continue; // cannot reduce further
        tmp_freq        = frequency_pstate_to_freq(tmp[i]);
        next_freq       = frequency_pstate_to_freq(tmp[i] - 1);
        logical_pstates = (next_freq - tmp_freq) / 100000;

        if (tmp[i] > target[i]) {
            /* *changes is from pstate 3 --> 2 or lower, *changes_max is from 2 --> 1 or 1 --> 0 */
            if (tmp[i] <= 2)
                *changes_max += 1;
            else
                *changes += logical_pstates;
        }
    }
}

/* Reduce 1 pstate in f, whith a limit */
static void reduce_one_pstate(uint *f, uint64_t *limit)
{
    int i;
    for (i = 0; i < MAX_CPUS_SUPPORTED; i++) {
        if (f[i] > limit[i])
            f[i]--;
    }
}

#define PSTATE_STEP  8
#define PSTATE0_STEP 30

// moved here from powercap_status.c as it is only used in dvfs cases
static uint32_t _compute_extra_power(uint current_power, uint max_steps, pstate_t *current, uint64_t *target)
{
    uint total = 0, changes, changes_max;
    uint tmp[MAX_CPUS_SUPPORTED];
    // no pstate_change
    if (_pstates_are_equal(current, target))
        return 0;
    // vector_print_pstates(current, node_size);
    // vector_print_pstates(target, node_size);

    memcpy(tmp, current, sizeof(uint) * MAX_CPUS_SUPPORTED);
    do {
        num_cpus_to_reduce_pstates(tmp, target, &changes, &changes_max);
        if (changes || changes_max) {
            total += (PSTATE_STEP * (float) changes / (float) node_size) +
                     (PSTATE0_STEP * (float) changes_max / (float) node_size);
            reduce_one_pstate(tmp, target);
        }
        max_steps--;

    } while ((changes || changes_max) &&
             (max_steps > 0)); // if no more changes can be done or we pass the max number of steps
    return total;
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
    if (!pc_on)
        return 0;
    /* Return 0 means we cannot release power */
    if (current_limit == POWER_CAP_UNLIMITED)
        return 0;

    status->ok         = PC_STATUS_OK;
    status->exceed     = 0;
    status->stress     = 0;
    status->requested  = 0;
    status->current_pc = current_limit;

    memset(c_req_f, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);
    pmgt_get_app_req_freq(DOMAIN_CPU, c_req_f, node_size);
    mgt_cpufreq_get_current_list(NULL, curr_pstates);

    status->stress =
        powercap_get_stress(av_pstates[0].khz, av_pstates[num_pstates - 1].khz, c_req_f[0], curr_pstates[0].khz);

    /* Compute current power */
    if (energy_cpu_read_copy(NULL, (ullong *) energy_ct2, (ullong *) energy_ct1, (ullong *) energy_diff) == EAR_ERROR) {
        debug("Error reading energy data");
        return 0;
    }
    timestamp_getfast(&curr_time);

    // DEBUG: Print all the energy values
    debug("=== Energy Debug Info ===");
    debug("energy_packs: %u", energy_packs);
    double time_diff_ms = timestamp_diff(&curr_time, &last_time, TIME_MSECS);
    double time_diff_s  = time_diff_ms / 1000.0;
    debug("Time difference: %.3f ms (%.6f s)", time_diff_ms, time_diff_s);

    double energy = 0;
    for (int i = 0; i < energy_packs; i++) {
        debug("Pack[%d]: ct1=%lu, ct2=%lu, diff=%lu", i, energy_ct1[i], energy_ct2[i], energy_diff[i]);
        energy += energy_diff[i];
    }
    float power = energy_cpu_compute_power(energy, time_diff_s);
    debug("Computed power: %.2f W", power);

    last_time = curr_time;

    debug("Current power: %.2f, current_limit: %u", power, current_limit);

    /* Running below target settings, requesting more power */
    if (_cmp_curr_pstate_with_target_freq(curr_pstates, c_req_f)) {
        status->ok        = PC_STATUS_GREEDY;
        status->requested = _compute_extra_power(power, num_pstates, av_pstates, c_req_f);
        return 0;
    }

    /* Calculate releasable power */
    debug("Current power: %.2f, current_limit: %u", power, current_limit);

    if (power >= current_limit) {
        /* Current power consumption meets or exceeds limit, cannot release power */
        debug("Current power %.2f >= limit %u, cannot release power", power, current_limit);
        return 0;
    }

    uint32_t ctbr = (current_limit - power) * 0.5;
    debug("Calculated releasable power: %u", ctbr);

    /*Running at settings but cannot release enough power, status OK */
    if (ctbr < MIN_CPU_POWER_MARGIN) {
        return 0;
    }

    /* Can release power */
    status->ok     = PC_STATUS_RELEASE;
    status->exceed = ctbr;
    return 1;
}
