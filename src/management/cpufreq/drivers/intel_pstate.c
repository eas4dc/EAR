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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1

#define _GNU_SOURCE
#include <sched.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/output/debug.h>
#include <common/utils/keeper.h>
#include <metrics/common/file.h>
#include <management/cpufreq/drivers/intel_pstate.h>

#define PATH_SMF   "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq" // line break
#define PATH_SGV   "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor" // line break
#define PATH_CMF0  "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"  // Test read
#define PATH_SMF0  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"  // Test write
#define PATH_SGV0  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"  // Test write

static uint    cpu_count;
static ullong *current_list;
static uint    avail_list_count;
static ullong  avail_list[128]; // From 10 GHz to 1 GHz there are 90 items, enough
static uint    boost_enabled;
static ullong  freq_max; // KHz
static ullong  freq_max0; // Initial max freq
static ullong  freq_base; // KHz
static ullong  freq_batman; // MAX to BASE if boost enabled
static uint    governor0;
static uint    governor_last = 0;
static int    *fds_smf; // FD scaling max freq
static int    *fds_sgv;

void mgt_intel_pstate_load(topology_t *tp, mgt_ps_driver_ops_t *ops)
{
    char buffer[SZ_PATH];
    int set_frequency = 1;
    int set_governor  = 1;
    int fd_cmf;

    // Files RW (at least, can we read the current frequency and governors?)
    if (!filemagic_can_read (PATH_CMF0, &fd_cmf )) return;
    set_frequency = filemagic_can_mwrite(PATH_SMF, tp->cpu_count, &fds_smf);
    set_governor  = filemagic_can_mwrite(PATH_SGV, tp->cpu_count, &fds_sgv);
    if (!set_frequency && !filemagic_can_mread(PATH_SMF, tp->cpu_count, &fds_smf)) return;
    if (!set_governor  && !filemagic_can_mread(PATH_SGV, tp->cpu_count, &fds_sgv)) return;
    // Opening cpuinfo_max_freq
    if ((fd_cmf = open(PATH_CMF0, O_RDONLY)) < 0) {
        return_msg(, strerror(errno));
    }
    // Reading the content of cpuinfo_max_freq
    if (!filemagic_word_read(fd_cmf, buffer, 0)) {
        debug("Opening '%s' failed: %d (%s)", PATH_CMF0, fd_cmf, strerror(errno));
        return_msg(, strerror(errno));
    }
    // Saving some required data
    cpu_count     = tp->cpu_count;
    boost_enabled = tp->boost_enabled;
    freq_base     = (ullong) tp->base_freq;
    freq_max      = (ullong) atoi(buffer);
    freq_max0     = freq_max;
    current_list  = calloc(cpu_count, sizeof(ullong));
    // Small patch for ARMs (provisional)
    if (tp->vendor == VENDOR_ARM) {
        freq_base = freq_max;
    }
    //
    mgt_intel_pstate_governor_get(&governor0);
    // Saved initial values
    keeper_macro(uint32, "IntelPstateDefaultGovernor", governor0);
    keeper_macro(uint64, "IntelPstateDefaultFrequency", freq_max0);
    debug("IntelPstateDefaultGovernor: %u", governor0);
    debug("IntelPstateDefaultFrequency: %llu", freq_max0);
    // Driver references
    apis_put(ops->init                 , mgt_intel_pstate_init                                );
    apis_put(ops->dispose              , mgt_intel_pstate_dispose                             );
    apis_put(ops->reset                , mgt_intel_pstate_reset                               );
    apis_put(ops->get_available_list   , mgt_intel_pstate_get_available_list                  );
    apis_put(ops->get_current_list     , mgt_intel_pstate_get_current_list                    );
    apis_put(ops->get_boost            , mgt_intel_pstate_get_boost                           );
    apis_put(ops->get_governor         , mgt_intel_pstate_governor_get                        );
    apis_put(ops->get_governor_list    , mgt_intel_pstate_governor_get_list                   );
    apis_put(ops->is_governor_available, mgt_intel_pstate_governor_is_available               );
    apis_pin(ops->set_current_list     , mgt_intel_pstate_set_current_list     , set_frequency);
    apis_pin(ops->set_current          , mgt_intel_pstate_set_current          , set_frequency);
    apis_pin(ops->set_governor         , mgt_intel_pstate_governor_set         , set_governor );
    apis_pin(ops->set_governor_mask    , mgt_intel_pstate_governor_set_mask    , set_governor );
    apis_pin(ops->set_governor_list    , mgt_intel_pstate_governor_set_list    , set_governor );
    debug("Loaded INTEL_PSTATE");
}

state_t mgt_intel_pstate_init()
{
    ullong m = 0;
    uint i = 0;

    debug("cpuinfo_max_freq: %llu", freq_max);
    debug("topo->base_freq : %llu", freq_base);
    // Discovering if boost is enabled
    if (!boost_enabled) {
        boost_enabled = (freq_max > freq_base);
        // A disguised frequency
        freq_batman = freq_max;
    }
    if (boost_enabled) {
        // Boost is marked as base clock + 1MHz
        freq_batman = freq_base + 1000;
        avail_list[0] = freq_batman;
        debug("avail_list%u: %llu (B)", 0, freq_batman);
        i = 1;
    }
    // While greater than 1GHz
    while((avail_list[i] = freq_base - (100000LLU*m)) >= 1000000) {
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

/* Getters */
state_t mgt_intel_pstate_get_available_list(const ullong **list, uint *list_count)
{
    static ullong shared_list[128];
    memcpy(shared_list, avail_list, sizeof(ullong)*128);
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
            s = EAR_ERROR;
        } else {
            current_list[cpu] = (ullong) atoi(data);
            if (boost_enabled && current_list[cpu] == freq_max) {
                current_list[cpu] = freq_batman;
            }
        }
        debug("current_list%d: %llu", cpu, current_list[cpu]);
    }
    *p = current_list;
    return s;
}

state_t mgt_intel_pstate_get_boost(uint *boost_enabled)
{
    *boost_enabled = 0;
	return EAR_SUCCESS;
}

/** Setters */
state_t mgt_intel_pstate_set_current_list(uint *freqs_index)
{
    char data[SZ_NAME_SHORT];
    state_t s = EAR_SUCCESS;
    int cpu;

    for (cpu = 0; cpu < cpu_count; ++cpu) {
        if (freqs_index[cpu] == ps_nothing) {
            continue;
        }
        if (freqs_index[cpu] > avail_list_count) {
            freqs_index[cpu] = avail_list_count-1;
        }
        sprintf(data, "%llu", avail_list[freqs_index[cpu]]);
        debug("set_list%d: %llu", cpu, avail_list[freqs_index[cpu]]);
        if (!filemagic_word_write(fds_smf[cpu], data, strlen(data), 1)) {
            s = EAR_ERROR;
        }
    }
    return s;
}

state_t mgt_intel_pstate_set_current(uint freq_index, int cpu)
{
    char data[SZ_NAME_SHORT];
    // Correcting invalid values
    if (freq_index >= avail_list_count) {
        freq_index = avail_list_count-1;
    }
    // If is one P_STATE for all CPUs
    if (cpu == all_cpus) {
        // Converting frequency to text
        sprintf(data, "%llu", avail_list[freq_index]);
        return (filemagic_word_mwrite(fds_smf, cpu_count, data, 1) ? EAR_SUCCESS:EAR_ERROR);
    }
    // If it is for a specified CPU
    if (cpu >= 0 && cpu < cpu_count) {
        sprintf(data, "%llu", avail_list[freq_index]);
        debug("writing a word '%s'", data);
        return (filemagic_word_write(fds_smf[cpu], data, strlen(data), 1) ? EAR_SUCCESS:EAR_ERROR);
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

state_t mgt_intel_pstate_governor_get(uint *governor)
{
    return static_get_governor(0, governor);
}

state_t mgt_intel_pstate_governor_get_list(uint *governors)
{
    state_t s;
    int i;
    for (i = 0; i < cpu_count; ++i) {
        if (state_fail(s = static_get_governor(i, governors))) {
            return s;
        }
    }
    return EAR_SUCCESS;
}

state_t mgt_intel_pstate_governor_set(uint governor)
{
    char buffer[64];
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
    if (state_fail(s = mgt_governor_tostr(governor, buffer))) {
        return s;
    }
    return (filemagic_word_mwrite(fds_sgv, cpu_count, buffer, 1) ? EAR_SUCCESS:EAR_ERROR);
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
    for (i = 0; i < cpu_count; ++i) {
        if (CPU_ISSET(i, &mask)) {
            if (!filemagic_word_write(fds_sgv[i], buffer, strlen(buffer), 1)) {
                s = EAR_ERROR;
            }
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
        if (!filemagic_word_write(fds_sgv[i], buffer, strlen(buffer), 1)) {
            return EAR_ERROR;
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
    mgt_intel_pstate_load(&tp, &ops);

    mgt_intel_pstate_init();

    if (argc == 1) {
        return 0;
    }
    if (atoi(argv[1]) == 1) {
        if (state_fail(mgt_intel_pstate_governor_set(Governor.performance))) {
            debug("mgt_intel_pstate_governor_set: %s", state_msg);
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
            debug("mgt_intel_pstate_governor_set: %s", state_msg);
        }
    }
    return 0;
}
#endif
