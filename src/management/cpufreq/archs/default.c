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

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/hardware/cpuid.h>
#include <common/hardware/bithack.h>
#include <management/cpufreq/archs/amd17.h>
#include <management/cpufreq/archs/default.h>

static topology_t           tp;
static mgt_ps_driver_ops_t *driver;
static const ullong        *freqs_available;
static uint                 freqs_count;
static uint                 pstate_nominal;
static uint                 boost_enabled;

state_t mgt_cpufreq_default_load(topology_t *tp_in, mgt_ps_ops_t *ops, mgt_ps_driver_ops_t *ops_driver)
{
	state_t s;
	int cond1;
	int cond2;

    /* If AMD : EARD will load amdxx and then this function will not replace
    * the sysops. If library, amdxx will fail and then with this new condition
    * the default will not load the sysops, allowing the eard class to load it.
    * If !AMD, then OS CPUFREQ, the eard will not load amdxx and will load
    * the default. The library will never load amdxx and will load default.
    */
    #if !AMD_OSCPUFREQ
    debug("Not using CPUFREQ frequencies");
    if (tp_in->vendor == VENDOR_AMD && tp_in->family >= FAMILY_ZEN) {
        return_msg(EAR_ERROR, Generr.api_incompatible);
    }
    #else
    debug("Using CPUFREQ frequencies");
    #endif
	if (ops_driver->init == NULL) {
		return_msg(EAR_ERROR, "Driver is not available");
	}
	if(state_fail(s = topology_copy(&tp, tp_in))) {
		return s;
	}
	// Setting our own driver variable
	driver = ops_driver;
	// Conditional 1, if can set frequencies
	cond1 = (driver->set_current_list != NULL);
	// Conditional 2, if can set governor
	cond2 = (driver->set_governor != NULL);
	// Setting references
	apis_put(ops->init,               mgt_cpufreq_default_init);
	apis_put(ops->dispose,            mgt_cpufreq_default_dispose);
    apis_put(ops->get_info,           mgt_cpufreq_default_get_info);
    apis_put(ops->get_freq_details,   mgt_cpufreq_default_get_freq_details);
    apis_put(ops->count_available,    mgt_cpufreq_default_count_available);
    apis_put(ops->get_available_list, mgt_cpufreq_default_get_available_list);
	apis_put(ops->get_current_list,   mgt_cpufreq_default_get_current_list);
	apis_put(ops->get_nominal,        mgt_cpufreq_default_get_nominal);
	apis_put(ops->get_index,          mgt_cpufreq_default_get_index);
	apis_pin(ops->set_current_list,   mgt_cpufreq_default_set_current_list, cond1);
	apis_pin(ops->set_current,        mgt_cpufreq_default_set_current, cond1);
	apis_put(ops->get_governor,       mgt_cpufreq_default_governor_get);
	apis_put(ops->get_governor_list,  mgt_cpufreq_default_governor_get_list);
	apis_pin(ops->set_governor,       mgt_cpufreq_default_governor_set, cond2);
	apis_pin(ops->set_governor_mask,  mgt_cpufreq_default_governor_set_mask, cond2);
	apis_pin(ops->set_governor_list,  mgt_cpufreq_default_governor_set_list, cond2);

	return EAR_SUCCESS;
}

static state_t static_dispose(state_t s, char *msg)
{
	driver->dispose();
	return_msg(s, msg);
}

state_t mgt_cpufreq_default_dispose(ctx_t *c)
{
	return static_dispose(EAR_SUCCESS, NULL);
}

state_t mgt_cpufreq_default_init(ctx_t *c)
{
	state_t s;

	debug("Initializing DEFAULT P_STATE control");
	if (state_fail(s = driver->init())) {
		return static_dispose(s, state_msg);
	}
	if (state_fail(s = driver->get_available_list(&freqs_available, &freqs_count))) {
		return static_dispose(s, state_msg);
	}
        #if SHOW_DEBUGS
        debug("Available processor frequencies:");
        int i;
        for (i = 0; i < freqs_count; ++i) {
                debug("PS%d: %llu KHz", i, freqs_available[i]);
        }
        #endif
	cpuid_regs_t r;
	// - Intel System Programming Guide Vol. 2A, Software Developer Manual 3-193,
	// INSTRUCTION SET REFERENCE, A-L, Thermal and Power Management Leaf.
	CPUID(r,6,0);
	// Getting the nominal P_STATE (boost enabled or not)
	boost_enabled  = getbits32(r.eax, 1, 1);
	pstate_nominal = boost_enabled;
	//
	debug("Initialized correctly with %d P_STATEs", freqs_count);

	return EAR_SUCCESS;
}

void mgt_cpufreq_default_get_info(apinfo_t *info)
{
    info->api = API_DEFAULT;
	info->devs_count = tp.cpu_count;
}

void mgt_cpufreq_default_get_freq_details(freq_details_t *details)
{
    if (driver->get_freq_details != NULL) {
        driver->get_freq_details(details);
    }
}

// Given a frequency, find its P_STATE (or closest).
state_t mgt_cpufreq_default_count_available(ctx_t *c, uint *pstate_count)
{
    *pstate_count = freqs_count;
    return EAR_SUCCESS;
}

static state_t static_get_index(ullong freq_khz, uint *pstate_index, uint closest)
{
	int pst;

	if (freq_khz == 0LLU) {
		return_msg(EAR_ERROR, "P_STATE not found for 0 KHz");
	}
	// Boost test
	if (boost_enabled && freqs_available[0] == freq_khz) {
		*pstate_index = 0;
		return EAR_SUCCESS;
	}
	// Closest test
	if (closest && freq_khz > freqs_available[pstate_nominal]) {
		*pstate_index = pstate_nominal;
		return EAR_SUCCESS;
	}
	// Searching
	for (pst = pstate_nominal; pst < freqs_count; ++pst)
	{
		debug("comparing frequencies %llu == %llu", freqs_available[pst], freq_khz);
		if (freq_khz == freqs_available[pst]) {
			*pstate_index = pst;
			return EAR_SUCCESS;
		}
		if (closest && freq_khz > freqs_available[pst]) {
			if (pst > pstate_nominal) {
				ullong aux_1 = freqs_available[pst-1] - freq_khz;
				ullong aux_0 = freq_khz - freqs_available[pst+0];
				pst = pst - (aux_1 < aux_0);
			}
			*pstate_index = pst;
			return EAR_SUCCESS;
		}
	}
	if (closest) {
		*pstate_index = freqs_count-1;
		return EAR_SUCCESS;
	}
	return_msg(EAR_ERROR, "P_STATE not found");
}

state_t mgt_cpufreq_default_get_available_list(ctx_t *c, pstate_t *pstate_list)
{
	int i;
	for (i = 0; i < freqs_count; ++i) {
		pstate_list[i].idx = (ullong) i;
		pstate_list[i].khz = freqs_available[i];
	}
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_default_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	const ullong *current_list;
	uint cpu, pst;
	state_t s;

	if (state_fail(s = driver->get_current_list(&current_list))) {
		return s;
	}
	for (cpu = 0; cpu < tp.cpu_count; ++cpu) {
		pstate_list[cpu].idx = 0;
		pstate_list[cpu].khz = freqs_available[0];

		if (state_ok(static_get_index(current_list[cpu], &pst, 0)))
		{
			pstate_list[cpu].idx = pst;
			pstate_list[cpu].khz = freqs_available[pst];
		}
	}

	return EAR_SUCCESS;
}

state_t mgt_cpufreq_default_get_nominal(ctx_t *c, uint *pstate_index)
{
	*pstate_index = pstate_nominal;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_default_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest)
{
	return static_get_index(freq_khz, pstate_index, closest);
}

/** Setters */
state_t mgt_cpufreq_default_set_current_list(ctx_t *c, uint *pstate_index)
{
	#if 0
	state_t s;
	// Too much robustness
	if (state_fail(s = driver->set_governor(Governor.userspace))) {
		return s;
	}
	#endif
	return driver->set_current_list(pstate_index);
}

state_t mgt_cpufreq_default_set_current(ctx_t *c, uint pstate_index, int cpu)
{
	#if 0
	state_t s;
	// Too much robustness
	if (state_fail(s = driver->set_governor(Governor.userspace))) {
		return s;
	}
	#endif
	return driver->set_current(pstate_index, cpu);
}

/** Governors */
state_t mgt_cpufreq_default_governor_get(ctx_t *c, uint *governor)
{
	return driver->get_governor(governor);
}

state_t mgt_cpufreq_default_governor_get_list(ctx_t *c, uint *governors)
{
	return driver->get_governor_list(governors);
}

state_t mgt_cpufreq_default_governor_set(ctx_t *c, uint governor)
{
	return driver->set_governor(governor);
}

state_t mgt_cpufreq_default_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask)
{
	return driver->set_governor_mask(governor, mask);
}

state_t mgt_cpufreq_default_governor_set_list(ctx_t *c, uint *governors)
{
	return driver->set_governor_list(governors);
}
