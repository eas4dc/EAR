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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef MANAGEMENT_CPUFREQ
#define MANAGEMENT_CPUFREQ

#include <common/states.h>
#include <common/plugins.h>
#include <common/hardware/topology.h>
#include <management/cpufreq/governor.h>
#include <metrics/common/pstate.h>

#define all_cpus -1

// The API
// 
// This API is designed to set the governor and frequency in the node. The 
// procedure is the usual: load, init, get/set and dispose. To check
// architecture limitations read the headers in archs folder.
//
// Props:
// 	- Thread safe: partially.
//	- User mode: no.
//
// Folders:
//	- archs: different node architectures, such as AMD and Intel.
//	- drivers: API contacting to different system drivers, such as acpi_cpufreq.
//	- recovery: old API.
//	- tests: examples.
//
// Compiling options:
//	- You can set DISABLE_AMD in archs/Makefile to disable the AMD17 loading,
//	  allowing the default or generic API selection.
//	- You can set RECOVERY in Makefile to replace current API by the old one in
//	  case of failure.
//
// Future work:
// 	- Offer a function to get frequency configurations between threads, cores,
// 	  or other kind of group of CPUs. In example:
// 	  	mgt_cpufreq_get_conf(thread, core, group);
// 	- Offer a function to serialize/marshall context to copy its internal data
// 	  (context) in user mode applications. In example:
// 	  	mgt_cpufreq_serialize(ctx_t *c, void **serialization, size_t *size);
// 	  Or maybe to force static sized context allocations.
//
// Use example:
//	- You can find an example in cpufreq/tests folder.

typedef struct mgt_ps_driver_ops_s
{
	state_t (*init)               (ctx_t *c);
	state_t (*dispose)            (ctx_t *c);
	state_t (*get_available_list) (ctx_t *c, const ullong **freq_list, uint *freq_count);
	state_t (*get_current_list)   (ctx_t *c, const ullong **freq_list);
	state_t (*get_boost)          (ctx_t *c, uint *boost_enabled);
	state_t (*get_governor)       (ctx_t *c, uint *governor);
	state_t (*set_current_list)   (ctx_t *c, uint *freq_index);
	state_t (*set_current)        (ctx_t *c, uint freq_index, int cpu);
	state_t (*set_governor)       (ctx_t *c, uint governor);
} mgt_ps_driver_ops_t;

typedef struct mgt_ps_ops_s
{
	state_t (*init)                 (ctx_t *c, mgt_ps_driver_ops_t *driver_ops);
	state_t (*init_user)            (ctx_t *c, mgt_ps_driver_ops_t *driver_ops, const ullong *freq_list, uint freq_count);
	state_t (*dispose)              (ctx_t *c);
	state_t (*count_available)      (ctx_t *c, uint *pstate_count);
	state_t (*get_available_list)   (ctx_t *c, pstate_t *pstate_list);
	state_t (*get_current_list)     (ctx_t *c, pstate_t *pstate_list);
	state_t (*get_nominal)          (ctx_t *c, uint *pstate_index);
	state_t (*get_governor)         (ctx_t *c, uint *governor);
	state_t (*get_index)            (ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);
	state_t (*set_current_list)     (ctx_t *c, uint *pstate_index);
	state_t (*set_current)          (ctx_t *c, uint pstate_index, int cpu);
	state_t (*set_governor)         (ctx_t *c, uint governor);
} mgt_ps_ops_t;

/** The first function to call, because discovers the system and sets the internal API. */
state_t mgt_cpufreq_load(topology_t *tp);

state_t mgt_cpufreq_get_api(uint *api);

/** The second function to call, initializes all the data. */
state_t mgt_cpufreq_init(ctx_t *c);

/** Not privileged init. If freq_list is NULL the API tries to initialize anyway. */
state_t mgt_cpufreq_init_user(ctx_t *c, const ullong *freq_list, uint freq_count);

/** Frees its allocated memory. */
state_t mgt_cpufreq_dispose(ctx_t *c);

// Data
/** Returns the number of available P_STATEs. */
state_t mgt_cpufreq_count_available(ctx_t *c, uint *pstate_count);

/** Allocates a list of available P_STATEs. */
state_t mgt_cpufreq_alloc_available(ctx_t *c, pstate_t **pstate_list, uint *pstate_count);

/** Allocates a list of current P_STATE per CPU. */
state_t mgt_cpufreq_alloc_current(ctx_t *c, pstate_t **pstate_list, uint **index_list, uint *both_count);

// Getters
/** Returns the available P_STATE (struct) list. The allocated list depends on the user. */
state_t mgt_cpufreq_get_available_list(ctx_t *c, pstate_t *pstate_list);

/** Returns the current P_STATE (struct) per CPU. */
state_t mgt_cpufreq_get_current_list(ctx_t *c, pstate_t *pstate_list);

/** Returns the nominal P_STATE index. */
state_t mgt_cpufreq_get_nominal(ctx_t *c, uint *pstate_index);

/** Gets the governor (check governor.h to translate its int id to string). */
state_t mgt_cpufreq_get_governor(ctx_t *c, uint *governor);

/** Given a frequency in KHz, returns its available P_STATE index. */
state_t mgt_cpufreq_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest);

// Setters
/** Sets a P_STATE and userspace governor per CPU. The allocated list depends on the user. */
state_t mgt_cpufreq_set_current_list(ctx_t *c, uint *pstate_index);

/** Set a P_STATE in specified CPU. Use all_cpus to set that P_STATE in all CPUs. */
state_t mgt_cpufreq_set_current(ctx_t *c, uint pstate_index, int cpu);

/** Sets the governor (take a look to Governor global variable. */
state_t mgt_cpufreq_set_governor(ctx_t *c, uint governor);

#endif //MANAGEMENT_PSTATE
