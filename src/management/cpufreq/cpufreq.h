/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_CPUFREQ_H
#define MANAGEMENT_CPUFREQ_H

#define _GNU_SOURCE
#include <sched.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>
#include <management/cpufreq/governor.h>
#include <management/cpufreq/priority.h>
#include <management/cpufreq/frequency.h>
#include <metrics/common/pstate.h>

// The API
// 
// This API is designed to set the governor and frequency in the node. The 
// procedure is the usual: load, init, get/set and dispose. To check
// architecture limitations read the headers in archs folder.

typedef struct freq_details_s {
    ullong freq_max; // Boost included
    ullong freq_min;
    ullong freq_base;
    ullong freq_nominal; // Normally the same than freq_base
} freq_details_t;

typedef struct mgt_ps_driver_ops_s
{
	state_t (*init)                  ();
    state_t (*dispose)               ();
    state_t (*reset)                 ();
    void    (*get_freq_details)      (freq_details_t *details);
	state_t (*get_available_list)    (const ullong **freq_list, uint *freq_count);
	state_t (*get_current_list)      (const ullong **freq_list);
	state_t (*get_boost)             (uint *boost_enabled);
	state_t (*set_current_list)      (uint *freq_index);
	state_t (*set_current)           (uint freq_index, int cpu);
	state_t (*get_governor)          (uint *governor);
	state_t (*get_governor_list)     (uint *governor);
	state_t (*set_governor)          (uint governor);
	state_t (*set_governor_mask)     (uint governor, cpu_set_t mask);
	state_t (*set_governor_list)     (uint *governor);
    state_t (*is_governor_available) (uint governor);
} mgt_ps_driver_ops_t;

typedef struct mgt_ps_ops_s
{
    void    (*get_info)             (apinfo_t *info);
	state_t (*init)                 (ctx_t *c);
	state_t (*dispose)              (ctx_t *c);
    void    (*get_freq_details)     (freq_details_t *details);
    state_t (*count_available)      (ctx_t *c, uint *pstate_count);
    state_t (*get_available_list)   (ctx_t *c, pstate_t *pstate_list);
	state_t (*get_current_list)     (ctx_t *c, pstate_t *pstate_list);
	state_t (*get_nominal)          (ctx_t *c, uint *pstate_index);
	state_t (*get_index)            (ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);
	state_t (*set_current_list)     (ctx_t *c, uint *pstate_index);
	state_t (*set_current)          (ctx_t *c, uint pstate_index, int cpu);
    state_t (*reset)                (ctx_t *c);
	state_t (*get_governor)         (ctx_t *c, uint *governor);
	state_t (*get_governor_list)    (ctx_t *c, uint *governor);
	state_t (*set_governor)         (ctx_t *c, uint governor);
	state_t (*set_governor_mask)    (ctx_t *c, uint governor, cpu_set_t mask);
	state_t (*set_governor_list)    (ctx_t *c, uint *governor);
} mgt_ps_ops_t;

/** The first function to call, because discovers the system and sets the internal API. */
state_t mgt_cpufreq_load(topology_t *tp, int eard);

/** Deprecated. Returns the loaded API. */
void mgt_cpufreq_get_api(uint *api);

void mgt_cpufreq_get_info(apinfo_t *info);

/** Deprecated. Returns the number of CPUs. */
void mgt_cpufreq_count_devices(ctx_t *c, uint *dev_count);

/** The second function to call, initializes all the data. */
state_t mgt_cpufreq_init(ctx_t *c);

/** Frees its allocated memory. */
state_t mgt_cpufreq_dispose(ctx_t *c);

void mgt_cpufreq_get_freq_details(freq_details_t *details);

/** Returns the available P_STATE (struct) list. The allocated list depends on the user. */
state_t mgt_cpufreq_get_available_list(ctx_t *c, const pstate_t **pstate_list, uint *pstate_count);

/** Returns the current P_STATE (struct) per CPU. */
state_t mgt_cpufreq_get_current_list(ctx_t *c, pstate_t *pstate_list);

/** Returns the nominal P_STATE index. */
state_t mgt_cpufreq_get_nominal(ctx_t *c, uint *pstate_index);

/** Given a frequency in KHz, returns its available P_STATE index. */
state_t mgt_cpufreq_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);

/** Sets a P_STATE and userspace governor per CPU. The allocated list depends on the user. */
state_t mgt_cpufreq_set_current_list(ctx_t *c, uint *pstate_index);

/** Set a P_STATE in specified CPU. Accepts 'all_cpus' to set that P_STATE in all CPUs. */
state_t mgt_cpufreq_set_current(ctx_t *c, uint pstate_index, int cpu);

/** Resets to the default configuration. */
state_t mgt_cpufreq_reset(ctx_t *c);

/** Allocates a list of current P_STATE per CPU (devices). */
void mgt_cpufreq_data_alloc(pstate_t **pstate_list, uint **index_list);

void mgt_cpufreq_data_print(pstate_t *ps_list, uint ps_count, int fd);

char *mgt_cpufreq_data_tostr(pstate_t *ps_list, uint ps_count, char *buffer, int length);

// Governor subsystem
#define mgt_cpufreq_get_governor(c, g) mgt_cpufreq_governor_get(c, g)
#define mgt_cpufreq_set_governor(c, g) mgt_cpufreq_governor_set(c, g)

/** Gets the current governor id (specifically CPU0 governor). */
state_t mgt_cpufreq_governor_get(ctx_t *c, uint *governor);

/** Gets the current governor for each GPU. */
state_t mgt_cpufreq_governor_get_list(ctx_t *c, uint *governor);

/** Sets a governor to all CPUs. */
state_t mgt_cpufreq_governor_set(ctx_t *c, uint governor);

/** Sets a governor to the CPUs whose bit in the mask is 1. */
state_t mgt_cpufreq_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask);

/** Sets a governor to the CPUs given a list of governors id. */
state_t mgt_cpufreq_governor_set_list(ctx_t *c, uint *governor);

/** Gets if governor is available to set. */
int mgt_cpufreq_governor_is_available(ctx_t *c, uint governor);

#endif //MANAGEMENT_CPUFREQ_H
