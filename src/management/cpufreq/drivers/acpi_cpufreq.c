/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#define _GNU_SOURCE
#include <common/output/debug.h>
#include <common/sizes.h>
#include <common/utils/keeper.h>
#include <management/cpufreq/cpufreq_base.h>
#include <management/cpufreq/drivers/acpi_cpufreq.h>
#include <management/cpufreq/drivers/intel_pstate.h>
#include <metrics/common/file.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define N_FREQS   128

#define PATH_SSS  "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_setspeed"             // Test write
#define PATH_SGV  "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor"             // Test write
#define PATH_SAG0 "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"   // Test read
#define PATH_SAF0 "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies" // Test read
#define PATH_CPB0 "/sys/devices/system/cpu/cpu0/cpufreq/cpb"                           // Test read
#define PATH_BST0 "/sys/devices/system/cpu/cpufreq/boost"                              // Test read

static uint    cpu_count;
static cfb_t   bf;
static uint    avail_list_count;
static ullong  avail_list_real[N_FREQS]; // From 10 GHz to 1 GHz there are 90 items, enough
static ullong  avail_list_disp[N_FREQS]; // Returned frequencies
static ullong *current_list;
static int    *fds_sgv;
static int    *fds_sss;
static uint    governor_last;
static uint    governor0;
static ullong  freq_max0;
static ullong  freq_min0;

static int have1000(ullong khz)
{
    ullong khz_cleaned = (khz / 10000LLU) * 10000LLU;
    if (khz_cleaned == khz) return 0;
    return ((khz - khz_cleaned) == 1000LLU);
}

static state_t build_frequencies_list(ullong freq_max, ullong freq_min)
{
    char buffer[PATH_MAX];
    int fd_saf0 = -1;
    ullong m    = 0;
    uint i      = 0;
    // Building frequency list
    if (filemagic_can_read(PATH_SAF0, &fd_saf0)) {
        while (filemagic_word_read(fd_saf0, buffer, 0)) {
            avail_list_real[i] = (ullong) atoll(buffer);
            avail_list_disp[i] = avail_list_real[i];
            debug("avail_list_real/disp[%u]: %llu/%llu", i, avail_list_real[i], avail_list_disp[i]);
            ++i;
        }
        close(fd_saf0);
    } else {
        if (bf.boost_enabled) {
            // The +1000 to indicate boost is only for display purpose, or when
            // we read that the maximum frequency has it. In that case the
            // driver works with the +1000 format.
            if (freq_max == 0LLU) {
                avail_list_real[0] = bf.frequency + 1000LLU;
                avail_list_disp[0] = avail_list_real[0];
            } else if (!have1000(freq_max)) {
                avail_list_real[0] = freq_max;
                avail_list_disp[0] = bf.frequency + 1000LLU;
            } else {
                avail_list_real[0] = freq_max;
                avail_list_disp[0] = avail_list_real[0];
            }
            debug("avail_list_real/disp[%2u]: %7llu/%7llu", i, avail_list_real[i], avail_list_disp[i]);
            i = 1;
        }
        if (freq_min == 0LLU) {
            freq_min = 1000000LLU; // 1 GHz
        }
        if (freq_min < 100000LLU) {
            freq_min = 100000LLU; // 100 MHz
        }
        while ((avail_list_real[i] = bf.frequency - (100000LLU * m)) >= freq_min) {
            avail_list_disp[i] = avail_list_real[i];
            debug("avail_list_real/disp[%2u]: %7llu/%7llu", i, avail_list_real[i], avail_list_disp[i]);
            ++m;
            ++i;
        }
    }
    avail_list_count = i;
    return EAR_SUCCESS;
}

static int is_userspace_available(char *buffer)
{
    int found_userspace =  0;
    int fd_sag0         = -1;

    if (!filemagic_can_read(PATH_SAG0, &fd_sag0))
        return -1;
    // Read if there is userspace governor, if not this is not a valid driver.
    while (!found_userspace && filemagic_word_read(fd_sag0, buffer, 0)) {
        found_userspace = (strncmp(buffer, Goverstr.userspace, 9) == 0);
        debug("read the word '%s' (found userspace? %d)", buffer, found_userspace);
    }
    close(fd_sag0);
    return found_userspace;
}

void mgt_acpi_cpufreq_load(topology_t *tp, mgt_ps_driver_ops_t *ops)
{
    char buffer[PATH_MAX];
    int set_frequency = 1;
    int set_governor  = 1;
    int is_userspace  = 0;

    // Testing if frequency/governor set files are accessible
    set_frequency = filemagic_can_mwrite(PATH_SSS, tp->cpu_count, &fds_sss);
    set_governor  = filemagic_can_mwrite(PATH_SGV, tp->cpu_count, &fds_sgv);
    if (!set_frequency && !filemagic_can_mread(PATH_SSS, tp->cpu_count, &fds_sss)) return;
    if (!set_governor  && !filemagic_can_mread(PATH_SGV, tp->cpu_count, &fds_sgv)) return;
    // This is a userspace driver (based on acpi_cpufreq model).
    switch (is_userspace_available(buffer)) {
    case -1:
        debug("Can not determine if driver have userspace governor");
        return;
    case 0:
        if (!set_governor) {
            debug("Can not determine if driver have userspace governor");
            return;
        }
        // Saving current CPU0 governor
        if (!filemagic_word_read(fds_sgv[0], buffer, 1)) {
            debug("Error when reading current governor");
            return;
        }
        // Some drivers shown a strange behaviour. Only showed that userspace is available
        // after setting userspace in 'scaling_governor' file.
        if (!filemagic_word_write(fds_sgv[0], Goverstr.userspace, strlen(Goverstr.userspace), 0)) {
            debug("Error occurred: %s", state_msg);
        }
        // If is not found again...
        is_userspace = (is_userspace_available(buffer) == 1);
        // Recovering last governor
        filemagic_word_write(fds_sgv[0], buffer, strlen(buffer), 0);
        // Cleaning
        if (!is_userspace) {
            debug("Current driver does not have userspace governor");
            free(fds_sss);
            free(fds_sgv);
            return;
        }
    }
    // Getting base frequency and boost
    cpufreq_base_init(tp, &bf);
    // Allocating space for current list
    cpu_count    = tp->cpu_count;
    current_list = calloc(cpu_count, sizeof(ullong));
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
    // Building list of frequencies
    if (state_fail(build_frequencies_list(freq_max0, freq_min0))) {
        return;
    }
    // Saving initial governor
    mgt_acpi_cpufreq_governor_get(&governor0);
    // Saved initial values
    keeper_macro(uint32, "AcpiCpufreqDefaultGovernor", governor0);
    debug("AcpiCpufreqDefaultGovernor: %u", governor0);
    // Driver references
    apis_put(ops->init              , mgt_acpi_cpufreq_init);
    apis_put(ops->dispose           , mgt_acpi_cpufreq_dispose);
    apis_put(ops->reset             , mgt_acpi_cpufreq_reset);
    apis_put(ops->get_freq_details  , mgt_acpi_cpufreq_get_freq_details);
    apis_put(ops->get_available_list, mgt_acpi_cpufreq_get_available_list);
    apis_put(ops->get_current_list  , mgt_acpi_cpufreq_get_current_list);
    apis_put(ops->get_boost         , mgt_acpi_cpufreq_get_boost);
    apis_put(ops->get_governor      , mgt_acpi_cpufreq_governor_get);
    apis_put(ops->get_governor_list , mgt_acpi_cpufreq_governor_get_list);
    apis_put(ops->is_governor_available, mgt_acpi_cpufreq_governor_is_available);
    apis_pin(ops->set_current_list  , mgt_acpi_cpufreq_set_current_list,  set_frequency);
    apis_pin(ops->set_current       , mgt_acpi_cpufreq_set_current,       set_frequency);
    apis_pin(ops->set_governor      , mgt_acpi_cpufreq_governor_set,      set_governor);
    apis_pin(ops->set_governor_mask , mgt_acpi_cpufreq_governor_set_mask, set_governor);
    apis_pin(ops->set_governor_list , mgt_acpi_cpufreq_governor_set_list, set_governor);
    debug("Loaded ACPI_CPUFREQ");
}

state_t mgt_acpi_cpufreq_init()
{
    return EAR_SUCCESS;
}

state_t mgt_acpi_cpufreq_dispose()
{
    return EAR_SUCCESS;
}

state_t mgt_acpi_cpufreq_reset()
{
    return EAR_SUCCESS;
}

void mgt_acpi_cpufreq_get_freq_details(freq_details_t *details)
{
    details->freq_base = bf.frequency;
    details->freq_max  = avail_list_real[0];
    details->freq_min  = avail_list_real[avail_list_count - 1];
}

/* Getters */
state_t mgt_acpi_cpufreq_get_available_list(const ullong **list, uint *list_count)
{
    static ullong shared_list[128];
    memcpy(shared_list, avail_list_disp, sizeof(ullong) * 128);
    if (list != NULL) {
        *list = (const ullong *) shared_list;
    }
    if (list_count != NULL) {
        *list_count = avail_list_count;
    }
    return EAR_SUCCESS;
}

state_t mgt_acpi_cpufreq_get_current_list(const ullong **p)
{
    state_t s = EAR_SUCCESS;
    char data[64];
    int cpu;

    for (cpu = 0; cpu < cpu_count; ++cpu) {
        if (!filemagic_word_read(fds_sss[cpu], data, 1)) {
            current_list[cpu] = 0LLU;
            s                 = EAR_ERROR;
        } else {
            current_list[cpu] = (ullong) atoll(data);
        }
        debug("current_list%d: %llu", cpu, current_list[cpu]);
    }
    *p = current_list;
    return s;
}

state_t mgt_acpi_cpufreq_get_boost(uint *boost_enabled)
{
    *boost_enabled = bf.boost_enabled;
    return EAR_SUCCESS;
}

/** Setters */
state_t mgt_acpi_cpufreq_set_current_list(uint *freqs_index)
{
    char data[SZ_NAME_SHORT];
    state_t s = EAR_SUCCESS;
    int cpu;

    for (cpu = 0; cpu < cpu_count; ++cpu) {
        if (freqs_index[cpu] == ps_nothing) {
            continue;
        }
        if (freqs_index[cpu] > avail_list_count) {
            freqs_index[cpu] = avail_list_count - 1;
        }
        sprintf(data, "%llu", avail_list_real[freqs_index[cpu]]);
        debug("set_list%d: %llu", cpu, avail_list_real[freqs_index[cpu]]);
        strcat(data, "\n");
        if (!filemagic_word_write(fds_sss[cpu], data, strlen(data), 0)) {
            s = EAR_ERROR;
        }
    }
    return s;
}

state_t mgt_acpi_cpufreq_set_current(uint freq_index, int cpu)
{
    char data[SZ_NAME_SHORT];
    // Correcting invalid values
    if (freq_index >= avail_list_count) {
        freq_index = avail_list_count - 1;
    }
    // If is one P_STATE for all CPUs
    if (cpu == all_cpus) {
        // Converting frequency to text
        sprintf(data, "%llu", avail_list_real[freq_index]);
        return (filemagic_word_mwrite(fds_sss, cpu_count, data, 1) ? EAR_SUCCESS : EAR_ERROR);
    }
    // If it is for a specified CPU
    if (cpu >= 0 && cpu < cpu_count) {
        sprintf(data, "%llu", avail_list_real[freq_index]);
        debug("writing a word '%s'", data);
        strcat(data, "\n");
        return (filemagic_word_write(fds_sss[cpu], data, strlen(data), 0) ? EAR_SUCCESS : EAR_ERROR);
    }
    return_msg(EAR_ERROR, Generr.cpu_invalid);
}

static state_t static_get_governor(int cpu, uint *governor)
{
    char buffer[64];
    if (!filemagic_word_read(fds_sgv[cpu], buffer, 1)) {
        buffer[0] = '\0';
    }
    return mgt_governor_toint(buffer, governor);
}

state_t mgt_acpi_cpufreq_governor_get(uint *governor)
{
    return static_get_governor(0, governor);
}

state_t mgt_acpi_cpufreq_governor_get_list(uint *governors)
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

state_t mgt_acpi_cpufreq_governor_set(uint governor)
{
    char buffer[64];
    uint current;
    state_t s;

    // If governor is the same
    if (state_ok(mgt_acpi_cpufreq_governor_get(&current))) {
        if (governor == current) {
            return EAR_SUCCESS;
        }
    }
    if (governor == Governor.last) {
        governor = governor_last;
    }
    // Saving last governor
    governor_last = current;
    if (state_fail(s = mgt_governor_tostr(governor, buffer))) {
        return s;
    }
    return (filemagic_word_mwrite(fds_sgv, cpu_count, buffer, 1) ? EAR_SUCCESS : EAR_ERROR);
}

state_t mgt_acpi_cpufreq_governor_set_mask(uint governor, cpu_set_t mask)
{
    char buffer[64];
    state_t s = EAR_SUCCESS;
    int cpu;

    if (state_fail(s = mgt_governor_tostr(governor, buffer))) {
        return s;
    }
    strcat(buffer, "\n");
    for (cpu = 0; cpu < cpu_count; ++cpu) {
        if (CPU_ISSET(cpu, &mask)) {
            // verbose(0,"Setting cpu %d to governor '%s'", cpu, buffer);
            if (!filemagic_word_write(fds_sgv[cpu], buffer, strlen(buffer), 0)) {
                s = EAR_ERROR;
            }
        }
    }
    return s;
}

state_t mgt_acpi_cpufreq_governor_set_list(uint *governors)
{
    char buffer[64];
    state_t s;
    int cpu;

    for (cpu = 0; cpu < cpu_count; ++cpu) {
        if (state_fail(s = mgt_governor_tostr(governors[cpu], buffer))) {
            return s;
        }
        strcat(buffer, "\n");
        if (!filemagic_word_write(fds_sgv[cpu], buffer, strlen(buffer), 0)) {
            return EAR_ERROR;
        }
    }
    return EAR_SUCCESS;
}

int mgt_acpi_cpufreq_governor_is_available(uint governor)
{
    return 1;
}

#if TEST
static const ullong *frequencies;
static uint governors[1024];
static char buffer[128];

int main(int argc, char *argv[])
{
    mgt_ps_driver_ops_t ops;
    topology_t tp;

    // Topology
    topology_init(&tp);
    // Management
    mgt_acpi_cpufreq_load(&tp, &ops);

    mgt_acpi_cpufreq_init();

    if (argc == 1) {
        return 0;
    }
    if (atoi(argv[1]) == 1) {
        if (state_fail(mgt_acpi_cpufreq_governor_set(Governor.performance))) {
            debug("mgt_acpi_cpufreq_governor_set: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 2) {
        if (state_fail(mgt_acpi_cpufreq_governor_set(Governor.powersave))) {
            debug("mgt_acpi_cpufreq_governor_set: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 3) {
        if (state_fail(mgt_acpi_cpufreq_governor_set(Governor.userspace))) {
            debug("mgt_acpi_cpufreq_governor_set: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 4) {
        if (state_fail(mgt_acpi_cpufreq_governor_get_list(governors))) {
            debug("mgt_acpi_cpufreq_governor_get_list: %s", state_msg);
        }
        mgt_governor_tostr(governors[0], buffer);
        printf("Governor[0]: %s\n", buffer);
    }
    if (atoi(argv[1]) == 11) {
        if (state_fail(mgt_acpi_cpufreq_set_current(0, all_cpus))) {
            debug("mgt_acpi_cpufreq_governor_set: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 12) {
        if (state_fail(mgt_acpi_cpufreq_set_current(1, all_cpus))) {
            debug("mgt_acpi_cpufreq_governor_set: %s", state_msg);
        }
    }
    if (atoi(argv[1]) == 13) {
        if (state_fail(mgt_acpi_cpufreq_get_current_list(&frequencies))) {
            debug("mgt_acpi_cpufreq_governor_set: %s", state_msg);
        }
    }
    return 0;
}
#endif
