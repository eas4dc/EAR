/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_CPUFREQ_FREQUENCY
#define MANAGEMENT_CPUFREQ_FREQUENCY

#define _GNU_SOURCE
#include <management/cpufreq/cpufreq.h>
#include <sched.h>

// This API is OBSOLETE
//
// This is the old frequency API but customized to use the new cpufreq API. Its
// purpose is to connect EAR project with the cpufreq API without having to
// adapt and test the old code. The old frequency API without cpufreq is in
// recovery folder. Take a look into cpufreq.h to learn about compiling options.

#define check_usrspace_gov_set(st, vrb_lvl)                                                                            \
    if (state_fail(st)) {                                                                                              \
        if (state_is(st, EAR_WARNING)) {                                                                               \
            verbose(vrb_lvl, "%s", state_msg);                                                                         \
        } else if (state_is(st, EAR_NOT_INITIALIZED)) {                                                                \
            verbose(vrb_lvl, "%sERROR%s %s", COL_RED, COL_CLR, state_msg);                                             \
        } else {                                                                                                       \
            verbose(vrb_lvl, "%sERROR%s Setting userspace governor.", COL_RED, COL_CLR);                               \
        }                                                                                                              \
    }

typedef struct governor {
    char name[128];
    ulong max_f;
    ulong min_f;
} governor_t;

// Initializes API with privileges (used in: eard.c/dvfs.c).
state_t frequency_init(uint eard);
//
state_t frequency_dispose();

// Returns the P_STATE count.
uint frequency_get_num_pstates();
// Returns the current frequency in a specific CPU (privileged).
ulong frequency_get_cpu_freq(uint cpu);
// Returns the current frequency up to N cpus (privileged).
ulong frequency_get_cpufreq_list(uint cpus, ulong *cpuf);
// Returns the nominal available frequency.
ulong frequency_get_nominal_freq();
// Returns the nominal available P_STATE.
ulong frequency_get_nominal_pstate();
// Returns a list of available frequencies.
ulong *frequency_get_freq_rank_list();
// Sets a frequency in a CPU (privileged).
ulong frequency_set_all_cpus(ulong freq_khz);
// Sets a frequency if a bit is set (privileged).
ulong frequency_set_with_mask(cpu_set_t *mask, ulong freq_khz);
// Sets a frequency per CPU depending on its value in the list (privileged).
ulong frequency_set_with_list(uint cpus, ulong *freq_list);
// Converts a P_STATE into frequency.
ulong frequency_pstate_to_freq(uint pstate_index);
// Converts a frequency into P_STATE.
uint frequency_freq_to_pstate(ulong freq_khz);
// Converts a P_STATE in a frequency by searching in a list.
ulong frequency_pstate_to_freq_list(uint pstate_index, ulong *freq_list, uint pstate_count);
// Converts a frequency into P_STATE by searching in a list.
uint frequency_freq_to_pstate_list(ulong freq_khz, ulong *freq_list, uint pstate_count);
// Sets the governor userspace (privileged).
state_t frequency_set_userspace_governor_all_cpus();
// Given a frequency returns if is valid.
int frequency_is_valid_frequency(ulong freq_khz);
// Given a P_STATE returns if is valid.
int frequency_is_valid_pstate(uint pstate);
// Given a frequency returns the closest P_STATE.
uint frequency_closest_pstate(ulong freq_khz);
// Given a frequency returns the closest frequency.
ulong frequency_closest_frequency(ulong freq_khz);
//
ulong frequency_closest_high_freq(ulong freq_khz, int pstate_minimum);

// Sets a specific governor (privileged).
void set_governor(governor_t *_governor);

void verbose_frequencies(int cpus, ulong *f);

void vector_print_pstates(uint *pstates, uint num_cpus);

#endif // MANAGEMENT_CPUFREQ_FREQUENCY