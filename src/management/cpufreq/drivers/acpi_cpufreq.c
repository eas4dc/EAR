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
#include <unistd.h>
#include <string.h>
#include <common/sizes.h>
#include <common/output/debug.h>
#include <common/utils/keeper.h>
#include <metrics/common/file.h>
#include <management/cpufreq/drivers/acpi_cpufreq.h>

#define N_FREQS 128

#define PATH_SSS   "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_setspeed"             // Test write
#define PATH_SGV   "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor"             // Test write
#define PATH_SAG0  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"   // Test read
#define PATH_SAF0  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies" // Test read
#define PATH_CPB0  "/sys/devices/system/cpu/cpu0/cpufreq/cpb"                           // Test read
#define PATH_BST0  "/sys/devices/system/cpu/cpufreq/boost"                              // Test read

static uint    cpu_count;
static uint    avail_list_count;
static ullong  avail_list[N_FREQS]; // From 10 GHz to 1 GHz there are 90 items, enough
static ullong *current_list;
static uint    boost_enabled;
static int    *fds_sgv;
static int    *fds_sss;
static ullong  freq_base; // KHz
static ullong  freq_min = 1000000LLU; // 1GHz
static uint    governor_last;
static uint    governor0;

void mgt_acpi_cpufreq_load(topology_t *tp, mgt_ps_driver_ops_t *ops)
{
	char buffer[PATH_MAX];
	int found_userspace =  0;
	int set_frequency   =  1;
	int set_governor    =  1;
    int fd_sag0         = -1;
    int fd_saf0         = -1;
    ullong m            =  0;
    uint i              =  0;

    // Files RW
    if (!filemagic_can_read(PATH_SAG0, &fd_sag0)) return;
    set_frequency = filemagic_can_mwrite(PATH_SSS, tp->cpu_count, &fds_sss);
    set_governor  = filemagic_can_mwrite(PATH_SGV, tp->cpu_count, &fds_sgv);
    if (!set_frequency && !filemagic_can_mread(PATH_SSS, tp->cpu_count, &fds_sss)) return;
    if (!set_governor  && !filemagic_can_mread(PATH_SGV, tp->cpu_count, &fds_sgv)) return;
    // Building frequency list
    if (filemagic_can_read(PATH_SAF0, &fd_saf0)) {
        while (!found_userspace && filemagic_word_read(fd_saf0, buffer, 0)) {
            avail_list[i] = (ullong) atoll(buffer);
            debug("avail_list%u: %llu", i, avail_list[i]);
            ++i;
        }
        close(fd_saf0);
    } else {
        avail_list[0] = freq_base;
        if (boost_enabled) {
            avail_list[0] = freq_base + 1000LLU;
            i = 1;
        }
        while((avail_list[i] = freq_base - (100000LLU*m)) >= freq_min) {
            debug("avail_list%u: %llu", i, avail_list[i]);
            ++m;
            ++i;
        }
    }
    avail_list_count = i;
    if (avail_list_count == 0) {
        return_msg(, "No frequencies found");
    }
	// Read if there is userspace governor
	while (!found_userspace && filemagic_word_read(fd_sag0, buffer, 0)) {
		found_userspace = (strncmp(buffer, "userspace", 9) == 0);
		debug("read the word '%s' (found userspace? %d)", buffer, found_userspace);
	}
	close(fd_sag0);
	if (!found_userspace) {
		return_msg(, "Current driver does not have userspace governor.");
	}
    // Is boost enabled?
    if (filemagic_once_read(PATH_BST0, buffer, sizeof(buffer))) {
        boost_enabled = atoi(buffer);
    } else if (filemagic_once_read(PATH_CPB0, buffer, sizeof(buffer))) {
        boost_enabled = atoi(buffer);
    } else {
        boost_enabled = tp->boost_enabled;
    }
    // Saving some required data
    cpu_count     = tp->cpu_count;
    freq_base     = (ullong) tp->base_freq;
    current_list  = calloc(cpu_count, sizeof(ullong));
    //
    mgt_acpi_cpufreq_governor_get(&governor0);
    // Saved initial values
    keeper_macro(uint32, "AcpiCpufreqDefaultGovernor", governor0);
    debug("AcpiCpufreqDefaultGovernor: %u", governor0);
	// Driver references
	apis_put(ops->init                 , mgt_acpi_cpufreq_init                                );
	apis_put(ops->dispose              , mgt_acpi_cpufreq_dispose                             );
    apis_put(ops->reset                , mgt_acpi_cpufreq_reset                               );
	apis_put(ops->get_available_list   , mgt_acpi_cpufreq_get_available_list                  );
	apis_put(ops->get_current_list     , mgt_acpi_cpufreq_get_current_list                    );
	apis_put(ops->get_boost            , mgt_acpi_cpufreq_get_boost                           );
	apis_put(ops->get_governor         , mgt_acpi_cpufreq_governor_get                        );
	apis_put(ops->get_governor_list    , mgt_acpi_cpufreq_governor_get_list                   );
    apis_put(ops->is_governor_available, mgt_acpi_cpufreq_governor_is_available               );
	apis_pin(ops->set_current_list     , mgt_acpi_cpufreq_set_current_list     , set_frequency);
	apis_pin(ops->set_current          , mgt_acpi_cpufreq_set_current          , set_frequency);
	apis_pin(ops->set_governor         , mgt_acpi_cpufreq_governor_set         , set_governor );
	apis_pin(ops->set_governor_mask    , mgt_acpi_cpufreq_governor_set_mask    , set_governor );
	apis_pin(ops->set_governor_list    , mgt_acpi_cpufreq_governor_set_list    , set_governor );
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

/* Getters */
state_t mgt_acpi_cpufreq_get_available_list(const ullong **list, uint *list_count)
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

state_t mgt_acpi_cpufreq_get_current_list(const ullong **p)
{
	state_t s = EAR_SUCCESS;
    char data[64];
	int cpu;

	for (cpu = 0; cpu < cpu_count; ++cpu) {
		if (!filemagic_word_read(fds_sss[cpu], data, 1)) {
			current_list[cpu] = 0LLU;
			s = EAR_ERROR;
		} else {
			current_list[cpu] = (ullong) atoll(data);
		}
        debug("current_list%d: %llu", cpu, current_list[cpu]);
	}
    *p = current_list;
	return s;
}

state_t mgt_acpi_cpufreq_get_boost(uint *boost_enabled_in)
{
    *boost_enabled_in = boost_enabled;
	return EAR_SUCCESS;
}

/** Setters */
state_t mgt_acpi_cpufreq_set_current_list(uint *freqs_index)
{
	char data[SZ_NAME_SHORT];
	state_t s = EAR_SUCCESS;
	int cpu;

	for (cpu = 0; cpu < cpu_count; ++cpu){
		if (freqs_index[cpu] == ps_nothing) {
			continue;
		}
		if (freqs_index[cpu] > avail_list_count) {
			freqs_index[cpu] = avail_list_count-1;
		}
		sprintf(data, "%llu", avail_list[freqs_index[cpu]]);
        debug("set_list%d: %llu", cpu, avail_list[freqs_index[cpu]]);
        if (!filemagic_word_write(fds_sss[cpu], data, strlen(data), 1)) {
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
		freq_index = avail_list_count-1;
	}
	// If is one P_STATE for all CPUs
	if (cpu == all_cpus) {
		// Converting frequency to text
		sprintf(data, "%llu", avail_list[freq_index]);
        return (filemagic_word_mwrite(fds_sss, cpu_count, data, 1) ? EAR_SUCCESS:EAR_ERROR);
	}
	// If it is for a specified CPU
	if (cpu >= 0 && cpu < cpu_count) {
		sprintf(data, "%llu", avail_list[freq_index]);
		debug("writing a word '%s'", data);
        return (filemagic_word_write(fds_sss[cpu], data, strlen(data), 1) ? EAR_SUCCESS:EAR_ERROR);
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
    return (filemagic_word_mwrite(fds_sgv, cpu_count, buffer, 1) ? EAR_SUCCESS:EAR_ERROR);
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
        if (!filemagic_word_write(fds_sgv[cpu], buffer, strlen(buffer), 1)) {
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
