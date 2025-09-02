/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#define _GNU_SOURCE
#include <common/config/config_def.h>
#include <common/output/debug.h>
#include <common/output/verbose.h>
#include <common/utils/keeper.h>
#include <fcntl.h>
#include <management/cpufreq/cpufreq_base.h>
#include <management/cpufreq/drivers/intel_pstate.h>
#include <metrics/common/file.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PATH_SMF  "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq" // line break
#define PATH_SGV  "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor" // line break
#define PATH_SMF0 "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"  // Test write
#define PATH_SNF0 "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"  // Test write
#define PATH_CMF0 "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"  // Test read
#define PATH_CNF0 "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq"  // Test read

static uint cpu_count;
static ullong *current_list;
static uint avail_list_count;
static ullong avail_list[128]; // From 10 GHz to 1 GHz there are 90 items, enough
static cfb_t bf;
static ullong freq_ps0;  // KHz
static ullong freq_max;  // KHz
static ullong freq_max0; // Initial max freq
static ullong freq_min0; // Initial min freq
static uint governor0;
static uint governor_last = 0;
static int *fds_smf; // FD scaling max freq
static int *fds_sgv;

void mgt_intel_pstate_load(topology_t *tp, mgt_ps_driver_ops_t *ops)
{
    int set_frequency = 1;
    int set_governor  = 1;

    // Check if other driver is already loaded
    if (apis_loaded(ops))
        return;
    // This is a scaling_max_freq file driver (based on intel_pstate model).
    if (!filemagic_exists(PATH_SMF0))
        return;
    // Checking max/min frequency
    if (!mgt_intel_pstate_read_cpuinfo(1, &freq_max0)) {
        if (!keeper_load_uint64("AcpiCpufreqMaxFrequency", &freq_max0)) {
            if (!mgt_intel_pstate_read_scaling(1, &freq_max0)) {
            }
        }
    }
    if (!mgt_intel_pstate_read_cpuinfo(0, &freq_min0)) {
        if (!keeper_load_uint64("AcpiCpufreqMinFrequency", &freq_min0)) {
            if (!mgt_intel_pstate_read_scaling(0, &freq_min0)) {
            }
        }
    }
    // It worked so we can save the value for next loads
    if (freq_max0 != 0LLU) keeper_save_uint64("AcpiCpufreqMaxFrequency", freq_max0);
    if (freq_min0 != 0LLU) keeper_save_uint64("AcpiCpufreqMinFrequency", freq_min0);
    if (freq_max0 == 0LLU) freq_max0 = avail_list[0];
    if (freq_min0 == 0LLU) freq_min0 = avail_list[avail_list_count - 1];
    // Set check
    set_frequency = filemagic_can_mwrite(PATH_SMF, tp->cpu_count, &fds_smf);
    set_governor  = filemagic_can_mwrite(PATH_SGV, tp->cpu_count, &fds_sgv);
    if (!set_frequency && !filemagic_can_mread(PATH_SMF, tp->cpu_count, &fds_smf)) return;
    if (!set_governor  && !filemagic_can_mread(PATH_SGV, tp->cpu_count, &fds_sgv)) return;
    // Getting base frequency and boost
    cpufreq_base_init(tp, &bf);
    // Saving some required data
    cpu_count    = tp->cpu_count;
    freq_max     = freq_max0;
    current_list = calloc(cpu_count, sizeof(ullong));
    //
    mgt_intel_pstate_governor_get(&governor0);
    keeper_macro(uint32, "IntelPstateDefaultGovernor", governor0);
    // Debugging
    debug("IntelPstateDefaultGovernor: %u", governor0);
    debug("IntelPstateMaxFrequency   : %llu", freq_max0);
    debug("IntelPstateMinFrequency   : %llu", freq_min0);
    debug("IntelPstateBaseFrequency  : %llu", bf.frequency);
    // Driver references
    apis_put(ops->init, mgt_intel_pstate_init);
    apis_put(ops->dispose, mgt_intel_pstate_dispose);
    apis_put(ops->reset, mgt_intel_pstate_reset);
    apis_put(ops->get_freq_details, mgt_intel_pstate_get_freq_details);
    apis_put(ops->get_available_list, mgt_intel_pstate_get_available_list);
    apis_put(ops->get_current_list, mgt_intel_pstate_get_current_list);
    apis_put(ops->get_boost, mgt_intel_pstate_get_boost);
    apis_put(ops->get_governor, mgt_intel_pstate_governor_get);
    apis_put(ops->get_governor_list, mgt_intel_pstate_governor_get_list);
    apis_put(ops->is_governor_available, mgt_intel_pstate_governor_is_available);
    apis_pin(ops->set_current_list, mgt_intel_pstate_set_current_list, set_frequency);
    apis_pin(ops->set_current, mgt_intel_pstate_set_current, set_frequency);
    apis_pin(ops->set_governor, mgt_intel_pstate_governor_set, set_governor);
    apis_pin(ops->set_governor_mask, mgt_intel_pstate_governor_set_mask, set_governor);
    apis_pin(ops->set_governor_list, mgt_intel_pstate_governor_set_list, set_governor);
    debug("Loaded INTEL_PSTATE");
}

state_t mgt_intel_pstate_init()
{
    ullong m = 0;
    uint i   = 0;

#if DYNAMIC_BOOST_MGT
    bf.boost_enabled = 1;
    debug("'Boost enabled' forced.");
#endif
    // Discovering if boost is enabled
    if (bf.boost_enabled) {
        // Boost is marked as base clock + 1MHz
        freq_ps0      = bf.frequency + 1000;
        avail_list[0] = freq_ps0;
        debug("avail_list%u: %llu (B)", 0, avail_list[0]);
        i = 1;
    }
    // While greater than 1GHz
    while ((avail_list[i] = bf.frequency - (100000LLU * m)) >= 1000000) {
        debug("avail_list%u: %llu", i, avail_list[i]);
        ++m;
        ++i;
    }
    avail_list_count = i;
    return EAR_SUCCESS;
}

state_t mgt_intel_pstate_dispose()
{
    return EAR_SUCCESS;
}

state_t mgt_intel_pstate_reset()
{
    return EAR_SUCCESS;
}

void mgt_intel_pstate_get_freq_details(freq_details_t *details)
{
    details->freq_base = bf.frequency;
    details->freq_max  = freq_max0;
    details->freq_min  = freq_min0;
}

/* Getters */
state_t mgt_intel_pstate_get_available_list(const ullong **list, uint *list_count)
{
    static ullong shared_list[128];
    memcpy(shared_list, avail_list, sizeof(ullong) * 128);
    if (list != NULL) {
        *list = (const ullong *) shared_list;
    }
    if (list_count != NULL) {
        *list_count = avail_list_count;
    }
    return EAR_SUCCESS;
}

state_t mgt_intel_pstate_get_current_list(const ullong **p)
{
    state_t s = EAR_SUCCESS;
    char data[64];
    int cpu;

    for (cpu = 0; cpu < cpu_count; ++cpu) {
        if (!filemagic_word_read(fds_smf[cpu], data, 1)) {
            current_list[cpu] = 0LLU;
            s                 = EAR_ERROR;
        } else {
            current_list[cpu] = (ullong) atoi(data);
            if (bf.boost_enabled && current_list[cpu] == freq_max) {
                current_list[cpu] = freq_ps0;
            }
        }
        debug("current_list%d: %llu", cpu, current_list[cpu]);
    }
    *p = current_list;
    return s;
}

state_t mgt_intel_pstate_get_boost(uint *boost_enabled)
{
    *boost_enabled = bf.boost_enabled;
    return EAR_SUCCESS;
}

/** Setters */
state_t mgt_intel_pstate_set_current(uint freq_index, int cpu)
{
    char data[SZ_NAME_SHORT];
    //
    if (freq_index == ps_nothing) {
        return EAR_SUCCESS;
    }
    // Correcting invalid values
    if (freq_index >= avail_list_count) {
        freq_index = avail_list_count - 1;
    }
    // If is one P_STATE for all CPUs
    if (cpu == all_cpus) {
        debug("All cpus: boost enabled: %u, freq index %u", bf.boost_enabled, freq_index);
        if (bf.boost_enabled && freq_index == 0) {
            sprintf(data, "%llu", freq_max0);
        } else {
            sprintf(data, "%llu", avail_list[freq_index]);
        }
        return (filemagic_word_mwrite(fds_smf, cpu_count, data, 1) ? EAR_SUCCESS : EAR_ERROR);
    }
    // If it is for a specified CPU
    if (cpu >= 0 && cpu < cpu_count) {
        if (bf.boost_enabled && freq_index == 0) {
            sprintf(data, "%llu", freq_max0);
        } else {
            sprintf(data, "%llu", avail_list[freq_index]);
        }
        debug("writing a word '%s' freq_index %u freq_max %llu", data, freq_index, freq_max0);
        strcat(data, "\n");
        return (filemagic_word_write(fds_smf[cpu], data, strlen(data), 0) ? EAR_SUCCESS : EAR_ERROR);
    }
    return_msg(EAR_ERROR, Generr.cpu_invalid);
}

state_t mgt_intel_pstate_set_current_list(uint *freqs_index)
{
    int cpu;
    for (cpu = 0; cpu < cpu_count; ++cpu) {
        mgt_intel_pstate_set_current(freqs_index[cpu], cpu);
    }
    return EAR_SUCCESS;
}

static state_t static_get_governor(int cpu, uint *governor)
{
    char buffer[64];
    if (!filemagic_word_read(fds_sgv[cpu], buffer, 1)) {
        buffer[0] = '\0';
    }
    debug("CPU%d %s", cpu, buffer);
    return mgt_governor_toint(buffer, governor);
}

state_t mgt_intel_pstate_governor_get(uint *governor)
{
    return static_get_governor(0, governor);
}

state_t mgt_intel_pstate_governor_get_list(uint *governors)
{
    state_t s;
    int i;
    for (i = 0; i < cpu_count; ++i) {
        if (state_fail(s = static_get_governor(i, &governors[i]))) {
            return s;
        }
    }
    return EAR_SUCCESS;
}

state_t mgt_intel_pstate_governor_set(uint governor)
{
    char buffer_govr[64];
    char buffer_freq[64];
    uint current;
    state_t s;

    if (state_ok(mgt_intel_pstate_governor_get(&current))) {
        if (governor == current) {
            return EAR_SUCCESS;
        }
    }
    // Saving last governor
    if (governor == Governor.last) {
        governor = governor_last;
    }
    governor_last = current;
    if (!mgt_intel_pstate_governor_is_available(governor)) {
        return_msg(EAR_ERROR, "Governor not available");
    }
    if (state_fail(s = mgt_governor_tostr(governor, buffer_govr))) {
        return s;
    }
    // Restoring max frequency
    sprintf(buffer_freq, "%llu", freq_max0);
    debug("CPU-1: restoring to %s KHz and setting governor '%s'", buffer_freq, buffer_govr);
    if (!filemagic_word_mwrite(fds_smf, cpu_count, buffer_freq, 1)) {
        // If fails, nothing by now
    }
    return (filemagic_word_mwrite(fds_sgv, cpu_count, buffer_govr, 1) ? EAR_SUCCESS : EAR_ERROR);
}

static state_t set_governor_single(int cpu, char *governor)
{
    char buffer[64];
    // Restoring max frequency
    sprintf(buffer, "%llu", freq_max0);
    debug("CPU%d: restoring to %s KHz and setting governor '%s'", cpu, buffer, governor);
    if (!filemagic_word_write(fds_smf[cpu], buffer, strlen(buffer), 1)) {
        // If fails, nothing by now
    }
    if (!filemagic_word_write(fds_sgv[cpu], governor, strlen(governor), 1)) {
        return EAR_ERROR;
    }
    return EAR_SUCCESS;
}

state_t mgt_intel_pstate_governor_set_mask(uint governor, cpu_set_t mask)
{
    char buffer[64];
    state_t s;
    int i;

    if (!mgt_intel_pstate_governor_is_available(governor)) {
        return_msg(EAR_ERROR, "Governor not available");
    }
    if (state_fail(s = mgt_governor_tostr(governor, buffer))) {
        return s;
    }
    // Adding a \n is not required because write functions are fixed
    for (i = 0; i < cpu_count; ++i) {
        if (CPU_ISSET(i, &mask)) {
            s = set_governor_single(i, buffer);
        }
    }
    return s;
}

state_t mgt_intel_pstate_governor_set_list(uint *governors)
{
    char buffer[64];
    state_t s;
    int i;

    for (i = 0; i < cpu_count; ++i) {
        if (!mgt_intel_pstate_governor_is_available(governors[i])) {
            return_msg(EAR_ERROR, "Governor not available");
        }
        if (state_fail(s = mgt_governor_tostr(governors[i], buffer))) {
            return s;
        }
        // Adding a \n is not required because write functions are fixed
        if (state_fail(s = set_governor_single(i, buffer))) {
            return s;
        }
    }
    return EAR_SUCCESS;
}

int mgt_intel_pstate_governor_is_available(uint governor)
{
    if (governor == Governor.performance) {
        return 1;
    } else if (governor == Governor.powersave) {
        return 1;
    } else if (governor == Governor.last) {
        return 1;
    }
    return 0;
}

int mgt_intel_pstate_read_cpuinfo(int max, ullong *khz)
{
    char *path_cpuinfo = (max) ? PATH_CMF0 : PATH_CNF0;
    char buffer[64];
    int fd = -1;

    if (!filemagic_can_read(path_cpuinfo, &fd) || !filemagic_word_read(fd, buffer, 0)) {
        return 0;
    }
    *khz = 0LLU | (ullong) atoi(buffer);
    close(fd);
    return 1;
}

int mgt_intel_pstate_read_scaling(int max, ullong *khz)
{
    char *path_scaling = (max) ? PATH_SMF0 : PATH_SNF0;
    char buffer[64];
    int fd = -1;

    if (!filemagic_can_read(path_scaling, &fd) || !filemagic_word_read(fd, buffer, 0)) {
        return 0;
    }
    *khz = 0LLU | (ullong) atoi(buffer);
    close(fd);
    return 1;
}

#if TEST1
static const ullong *frequencies;
static uint governors[1024];
static char buffer[128];
static mgt_ps_driver_ops_t ops;
static topology_t tp;

int main(int argc, char *argv[])
{
    // Topology
    topology_init(&tp);
    // Management
    mgt_intel_pstate_load(&tp, &ops);
    mgt_intel_pstate_init();

    if (argc == 1) {
        return 0;
    }
    if (atoi(argv[1]) == 1) {
        if (state_fail(mgt_intel_pstate_governor_set(Governor.performance))) {
            debug("mgt_intel_pstate_governor_set error: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 2) {
        if (state_fail(mgt_intel_pstate_governor_set(Governor.powersave))) {
            debug("mgt_intel_pstate_governor_set: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 3) {
        if (state_fail(mgt_intel_pstate_governor_set(Governor.last))) {
            debug("mgt_intel_pstate_governor_set: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 4) {
        if (state_fail(mgt_intel_pstate_governor_get_list(governors))) {
            debug("mgt_intel_pstate_governor_get_list: %s", state_msg);
        }
        mgt_governor_tostr(governors[0], buffer);
        printf("Governor[0]: %s\n", buffer);
    }
    if (atoi(argv[1]) == 10) {
        if (state_fail(mgt_intel_pstate_set_current(0, all_cpus))) {
            debug("mgt_intel_pstate_governor_set: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 11) {
        if (state_fail(mgt_intel_pstate_set_current(1, all_cpus))) {
            debug("mgt_intel_pstate_governor_set: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 12) {
        if (state_fail(mgt_intel_pstate_get_current_list(&frequencies))) {
            debug("mgt_intel_pstate_governor_set failed: %s", state_msg);
        }
    }
    return 0;
}
#endif
