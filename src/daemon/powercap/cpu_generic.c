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

#include <metrics/cpufreq/cpufreq.h>
#include <metrics/energy_cpu/energy_cpu.h>

#include <management/cpufreq/cpufreq.h>
#include <management/cpupow/cpupow.h>

#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>
#include <daemon/powercap/powercap_status_conf.h>

/* CPU domain plugin for CPUs with a socket power limit, through management/cpupow:
 * RAPL on Intel (sockets and DRAM), HSMP on AMD (sockets only, no DRAM limit). */

static topology_t node_desc;
static uint32_t current_limit = POWER_CAP_UNLIMITED;
static uint pc_on             = 0;
static uint num_pstates       = 0;
static uint current_status    = 0;
static float cpu_weight       = 0;
static float dram_weight      = 0;
static int32_t num_cpus       = 0;
static int32_t num_dram       = 0;
/* DRAM has its own power limit (Intel RAPL). Without one (AMD: HSMP caps the socket only,
 * the DIMMs are outside it) DRAM is neither capped nor given a share of the limit. */
static bool dram_capable     = false;
static int32_t node_size     = 0;
static uint32_t energy_packs = 0;
static uint32_t *cpu_power   = NULL;
static uint32_t *dram_power  = NULL;
/* Power limit range per device, as cpupow reports it (0 = unknown) */
static uint32_t *cpu_min = NULL, *cpu_max = NULL, *dram_min = NULL, *dram_max = NULL;
/* CPU domain: power vs average effective frequency, and the most it can be capped at */
static power_estimator_t cpu_est = {0};
static uint32_t cpu_domain_max_w = 0;
static uint64_t *c_req_f         = NULL;
static uint64_t *energy_ct1      = NULL;
static uint64_t *energy_ct2      = NULL;
static uint64_t *energy_diff     = NULL;
static pstate_t *av_pstates      = NULL;
static pstate_t *curr_pstates    = NULL;
/* Effective frequency (APERF/MPERF): the cpufreq driver only reports the requested one */
static cpufreq_t *eff_last   = NULL;
static cpufreq_t *eff_curr   = NULL;
static ulong *eff_khz        = NULL;
static pstate_t *eff_pstates = NULL;
static bool eff_valid        = false; /* eff_pstates hold the last window's reading */
static timestamp last_time   = {0};
static timestamp curr_time   = {0};
static float last_power      = -1;
/* Shortest window power is measured over. Half the monitor period, so clock and
 * scheduling jitter can't make a tick (~500 ms apart) skip its refresh: every tick
 * measures, calls in between (the same tick's second pass, EARGM every 10-30 s) reuse
 * a value at most one period old. Without the monitor (no GPUs) EARGM is the only
 * caller and every call measures over its whole period */
#define POWER_WINDOW_MS (CHECK_STATUS_PERIOD / 2)

static suscription_t *sus_cpu_pc;

static domain_settings_t settings = {.node_ratio = 0.1, .security_range = 0.01}; //

state_t disable()
{
    if (dram_capable)
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
    cpufreq_load(&node_desc, NO_EARD);
    cpufreq_data_alloc(&eff_last, &eff_khz);
    cpufreq_data_alloc(&eff_curr, NULL);
    mgt_cpufreq_data_alloc(&eff_pstates, NULL);
    cpufreq_read(eff_last);
    c_req_f   = calloc(MAX_CPUS_SUPPORTED, sizeof(uint64_t));
    node_size = node_desc.cpu_count;

    /* energy_cpu init */
    energy_cpu_data_alloc(NULL, (ullong **) &energy_ct1, &energy_packs);
    energy_cpu_data_alloc(NULL, (ullong **) &energy_ct2, NULL);
    energy_cpu_data_alloc(NULL, (ullong **) &energy_diff, NULL);
    energy_cpu_read(NULL, (ullong *) energy_ct1);
    timestamp_getfast(&last_time);

    debug("Energy monitoring initialized: %u packs", energy_packs);
    for (int i = 0; i < energy_packs; i++) {
        debug("Initial energy_ct1[%d] = %lu", i, energy_ct1[i]);
    }

    /* Get TDP and calculate CPU/DRAM weight*/
    uint32_t total_cpu_power  = 0;
    uint32_t total_dram_power = 0;

    num_cpus     = mgt_cpupow_count_devices(CPUPOW_SOCKET);
    num_dram     = mgt_cpupow_count_devices(CPUPOW_DRAM);
    dram_capable = mgt_cpupow_powercap_is_capable(CPUPOW_DRAM);

    debug("num cpus: %u num_dram: %u", num_cpus, num_dram);

    cpu_power  = calloc(num_cpus, sizeof(uint32_t));
    dram_power = calloc(num_dram, sizeof(uint32_t));

    mgt_cpupow_tdp_get(CPUPOW_SOCKET, cpu_power);
    mgt_cpupow_tdp_get(CPUPOW_DRAM, dram_power);
    cpu_min  = calloc(num_cpus, sizeof(uint32_t));
    cpu_max  = calloc(num_cpus, sizeof(uint32_t));
    dram_min = calloc(num_dram, sizeof(uint32_t));
    dram_max = calloc(num_dram, sizeof(uint32_t));
    mgt_cpupow_limits_get(CPUPOW_SOCKET, cpu_min, cpu_max);
    mgt_cpupow_limits_get(CPUPOW_DRAM, dram_min, dram_max);
    /* range from the hardware (RAPL POWER_INFO, HSMP), the TDP as maximum when it reports none;
     * with neither the domain has no known maximum */
    powercap_max_or_tdp(cpu_max, cpu_power, num_cpus);
    powercap_max_or_tdp(dram_max, dram_power, num_dram);
    uint32_t pkg_min_w = 0, pkg_max_w = 0, dram_max_w = 0;
    int max_known = 1;
    for (int32_t i = 0; i < num_cpus; i++) {
        pkg_min_w += cpu_min[i];
        pkg_max_w += cpu_max[i];
        max_known &= (cpu_max[i] > 0);
    }
    for (int32_t i = 0; dram_capable && i < num_dram; i++) {
        dram_max_w += dram_max[i];
        max_known &= (dram_max[i] > 0);
    }
    cpu_domain_max_w = max_known ? pkg_max_w + dram_max_w : UINT32_MAX;
    power_estimator_init(&cpu_est, pkg_min_w, pkg_max_w, av_pstates[num_pstates - 1].khz, av_pstates[0].khz);

    for (int32_t i = 0; i < num_cpus; i++) {
        total_cpu_power += cpu_power[i];
    }
    /* without a DRAM limit the whole CPU domain share goes to the sockets */
    for (int32_t i = 0; dram_capable && i < num_dram; i++) {
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

/* Clamps a device cap to its hardware range when known (the firmware would hold it there anyway) */
static uint32_t _clamp_device(uint32_t w, uint32_t wmin, uint32_t wmax)
{
    if (wmax && w > wmax)
        w = wmax;
    if (wmin && w < wmin)
        w = wmin;
    return w;
}

static state_t _set_per_device_powercap_value(uint pid, uint32_t *limit, uint32_t *cpu_util)
{
    for (int32_t i = 0; i < num_cpus; i++) {
        /* cpupow gets POWERCAP_DISABLE, the storage keeps POWER_CAP_UNLIMITED as verify reads it;
         * the caller's array is left alone */
        cpu_power[i] =
            (limit[i] == POWER_CAP_UNLIMITED) ? POWERCAP_DISABLE : _clamp_device(limit[i], cpu_min[i], cpu_max[i]);
        powercap_set_stored_device_value(DOMAIN_CPU, i, (limit[i] == POWER_CAP_UNLIMITED) ? limit[i] : cpu_power[i]);
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
        powercap_update_all_device_storage(DOMAIN_CPU, POWER_CAP_UNLIMITED);
        if (dram_capable) {
            powercap_update_all_device_storage(DOMAIN_DRAM, POWER_CAP_UNLIMITED);
            mgt_cpupow_powercap_reset(CPUPOW_DRAM, RESET_DEFAULT);
        }
        state_t ret = mgt_cpupow_powercap_reset(CPUPOW_SOCKET, RESET_DEFAULT);
        if (state_ok(ret))
            current_limit = POWER_CAP_UNLIMITED; // the next real limit must be applied again
        return ret;
    }

    /* a device cap of 0 W, or 1 W (the UNLIMITED sentinel), is never programmed */
    for (int32_t i = 0; i < num_cpus; i++) {
        cpu_power[i] = _clamp_device(ear_max((uint32_t) ((limit * cpu_weight) / num_cpus), 2), cpu_min[i], cpu_max[i]);
        powercap_set_stored_device_value(DOMAIN_CPU, i, cpu_power[i]);
        debug("power assigned to cpu%d: %u", i, cpu_power[i]);
    }
    for (int32_t i = 0; dram_capable && i < num_dram; i++) {
        dram_power[i] =
            _clamp_device(ear_max((uint32_t) ((limit * dram_weight) / num_dram), 2), dram_min[i], dram_max[i]);
        powercap_set_stored_device_value(DOMAIN_DRAM, i, dram_power[i]);
        debug("power assigned to dram%d: %u", i, dram_power[i]);
    }

    state_t ret = mgt_cpupow_powercap_set(CPUPOW_SOCKET, cpu_power);
    if (ret == EAR_ERROR) {
        debug("error setting CPU powercap");
        return ret;
    }
    if (dram_capable) {
        ret = mgt_cpupow_powercap_set(CPUPOW_DRAM, dram_power);
        if (ret == EAR_ERROR) {
            debug("error setting DRAM powercap");
            return ret; // current_limit unchanged: the same limit is retried
        }
    }
    current_limit = limit;

    return ret;
}

/* Per-device DRAM caps (one per socket), called by pmgt when no DRAM plugin is loaded:
 * this plugin caps DRAM along with the sockets. */
state_t set_dram_powercap_value(uint32_t *limit)
{
    if (!dram_capable) {
        warning("DRAM powercap not supported on this CPU, DRAM values ignored");
        return EAR_ERROR;
    }
    for (int32_t i = 0; i < num_dram; i++) {
        dram_power[i] =
            (limit[i] == POWER_CAP_UNLIMITED) ? POWERCAP_DISABLE : _clamp_device(limit[i], dram_min[i], dram_max[i]);
        debug("power assigned to dram%d: %u", i, dram_power[i]);
    }
    state_t ret = mgt_cpupow_powercap_set(CPUPOW_DRAM, dram_power);
    if (state_fail(ret)) {
        debug("error setting DRAM powercap");
        return ret;
    }
    /* the storage keeps only what the hardware accepted */
    for (int32_t i = 0; i < num_dram; i++)
        powercap_set_stored_device_value(DOMAIN_DRAM, i, (limit[i] == POWER_CAP_UNLIMITED) ? limit[i] : dram_power[i]);
    return ret;
}

state_t get_dram_powercap_value(uint32_t *powercap)
{
    if (!dram_capable)
        return EAR_ERROR;
    return mgt_cpupow_powercap_get(CPUPOW_DRAM, powercap);
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

uint get_powercap_strategy()
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

/* Converts, per CPU, the P-state it runs at and its target frequency (kHz) to P-state
 * indices (0 = fastest) */
static void _freqs_to_pstate_indices(pstate_t *run_pstates, uint64_t *target_khz, uint *curr_idx, uint *target_idx)
{
    for (int32_t i = 0; i < node_size && i < MAX_CPUS_SUPPORTED; i++) {
        uint32_t t_pstate = 0;
        mgt_cpufreq_get_index(NULL, target_khz[i], &t_pstate, true);
        curr_idx[i]   = run_pstates[i].idx;
        target_idx[i] = t_pstate;
    }
}

/* Returns true if some CPU runs slower (higher P-state index) than its target */
static bool _below_target(uint *curr_idx, uint *target_idx)
{
    for (int32_t i = 0; i < node_size && i < MAX_CPUS_SUPPORTED; i++) {
        if (curr_idx[i] > target_idx[i])
            return true;
    }
    return false;
}

/* Average effective and average target frequency of the node's CPUs (kHz) */
static void _avg_khz(pstate_t *run, uint64_t *target_khz, ulong *eff, ulong *tgt)
{
    uint64_t e = 0, t = 0;
    int32_t n = ear_min(node_size, MAX_CPUS_SUPPORTED);
    for (int32_t i = 0; i < n; i++) {
        e += run[i].khz;
        t += target_khz[i];
    }
    *eff = 0;
    *tgt = 0;
    if (n > 0) {
        *eff = e / n;
        *tgt = t / n;
    }
}

#define MIN_CPU_POWER_MARGIN 10
/* Power above this fraction of the limit means the cap is what holds the CPUs back */
#define POWER_AT_LIMIT 0.97

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
    status->ok         = PC_STATUS_OK;
    status->exceed     = 0;
    status->stress     = 0;
    status->requested  = 0;
    status->current_pc = current_limit;
    if (!pc_on)
        return 0;
    /* Return 0 means we cannot release power */
    if (current_limit == POWER_CAP_UNLIMITED)
        return 0;

    memset(c_req_f, 0, sizeof(ulong) * MAX_CPUS_SUPPORTED);
    pmgt_get_app_req_freq(DOMAIN_CPU, c_req_f, node_size);
    mgt_cpufreq_get_current_list(NULL, curr_pstates);

    /* Compute current power and effective frequency over the same window. The monitor
     * asks twice in the same tick (CHECK_ASK_DEFAULT then CHECK_POWER_RED) and EARGM
     * asks at any time: timestamp_getfast() is CLOCK_MONOTONIC_COARSE, so a back-to-back
     * call sees no elapsed time and reads 0 W. Both are reused until POWER_WINDOW_MS has
     * elapsed, so "below target" and "power at the limit" judge the same interval */
    timestamp_getfast(&curr_time);
    double time_diff_ms = timestamp_diff(&curr_time, &last_time, TIME_MSECS);
    if (time_diff_ms >= POWER_WINDOW_MS || last_power < 0) {
        /* power limit throttling only shows in the effective frequency: judge it from APERF/MPERF */
        ulong eff_avg = 0;
        eff_valid     = eff_khz != NULL && state_ok(cpufreq_read_copy(eff_curr, eff_last, eff_khz, &eff_avg));
        if (eff_valid) {
            for (int32_t i = 0; i < node_size && i < MAX_CPUS_SUPPORTED; i++) {
                uint idx = 0;
                mgt_cpufreq_get_index(NULL, eff_khz[i], &idx, true);
                eff_pstates[i].idx = idx;
                eff_pstates[i].khz = eff_khz[i];
            }
        }
        if (energy_cpu_read_copy(NULL, (ullong *) energy_ct2, (ullong *) energy_ct1, (ullong *) energy_diff) ==
            EAR_ERROR) {
            debug("Error reading energy data");
            return 0;
        }
        double energy = 0;
        for (int i = 0; i < energy_packs; i++)
            energy += energy_diff[i];
        last_power = energy_cpu_compute_power(energy, time_diff_ms / 1000.0);
        last_time  = curr_time;
        ulong eff_khz_avg, tgt_khz_avg;
        _avg_khz(eff_valid ? eff_pstates : curr_pstates, c_req_f, &eff_khz_avg, &tgt_khz_avg);
        power_estimator_sample(&cpu_est, last_power, eff_khz_avg);
    }
    pstate_t *run_pstates = eff_valid ? eff_pstates : curr_pstates;

    status->stress =
        powercap_get_stress(av_pstates[0].khz, av_pstates[num_pstates - 1].khz, c_req_f[0], run_pstates[0].khz);

    float power = last_power;

    debug("Current power: %.2f, current_limit: %u", power, current_limit);

    /* Running below target settings because of the cap (power at the limit): requesting more power */
    uint cur_idx[MAX_CPUS_SUPPORTED], tgt_idx[MAX_CPUS_SUPPORTED];
    _freqs_to_pstate_indices(run_pstates, c_req_f, cur_idx, tgt_idx);
    if (_below_target(cur_idx, tgt_idx) && power >= current_limit * POWER_AT_LIMIT) {
        /* the watts the target frequency costs, from the power drawn now */
        ulong eff_khz_avg, tgt_khz_avg;
        _avg_khz(run_pstates, c_req_f, &eff_khz_avg, &tgt_khz_avg);
        double at_target = power_estimator_at(&cpu_est, power, eff_khz_avg, ear_max(tgt_khz_avg, eff_khz_avg));
        uint need        = (at_target > current_limit) ? (uint) (at_target - current_limit) : 0;
        uint requested   = powercap_device_request(cpu_domain_max_w, current_limit, need);
        /* at the domain maximum more watts can't help: not greedy */
        if (requested) {
            status->ok        = PC_STATUS_GREEDY;
            status->requested = requested;
            return 0;
        }
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
