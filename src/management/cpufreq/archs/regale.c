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
#include <common/utils/serial_buffer.h>
#include <metrics/common/regale.h>
#include <management/cpufreq/archs/regale.h>

static uint                 cpu_count;
static mgt_ps_driver_ops_t *driver;
static pstate_t            *available_list;
static uint                 available_count;
static uint                 boost_enabled;
static uint                 idx_nominal;

#define RGLE_MGT_CPUFREQ_GET_API          0
#define RGLE_MGT_CPUFREQ_GET_AVAILABLE    1
#define RGLE_MGT_CPUFREQ_GET_CURRENT      2
#define RGLE_MGT_CPUFREQ_GET_NOMINAL      3
#define RGLE_MGT_CPUFREQ_GET_GOVERNOR     4
#define RGLE_MGT_CPUFREQ_SET_CURRENT_LIST 5
#define RGLE_MGT_CPUFREQ_SET_CURRENT      6
#define RGLE_MGT_CPUFREQ_SET_GOVERNOR     7

state_t regale_rpc(uint call, uint in1, uint in2, uint *out);

regale_state_t regale_nm_cpufreq_count_available(uint *count);
regale_state_t regale_nm_cpufreq_get_available(uint idx, uint *freq_khz);
regale_state_t regale_nm_cpufreq_get_nominal(uint *idx);
regale_state_t regale_nm_cpufreq_get_current(uint cpu, uint *freq_khz);
regale_state_t regale_nm_cpufreq_set_current(uint cpu, uint idx);
regale_state_t regale_nm_governor_get(uint cpu, uint idx);
regale_state_t regale_nm_governor_set(uint cpu, uint *idx);

state_t mgt_cpufreq_regale_load(topology_t *tp_in, mgt_ps_ops_t *ops, mgt_ps_driver_ops_t *ops_driver, int regale)
{
	uint eard_api = 0;
    uint i, khz;
	state_t s;

	if (!regale) {
		return_msg(EAR_ERROR, "Regale's Node Manager not required");
	}
	if (!regale_connected()) {
		return_msg(EAR_ERROR, "Regale's Node Manager is not connected");
	}
    //
    cpu_count = tp_in->cpu_count;
    driver    = ops_driver;
    // Get available frequencies
    regale_nm_cpufreq_count_available(&available_count);
    available_list = calloc(available_count, sizeof(pstate_t));
    debug("Available processor frequencies:");
    for (i = 0; i < available_count; ++i) {
        available_list[i].idx = i;
        regale_nm_cpufreq_get_available(i, &khz);
        available_list[i].khz = (ullong) khz;
        debug("PS%d: %llu KHz", i, available_list[i].khz);
    }
    // Get nominal frequency
    regale_nm_cpufreq_get_nominal(&i);
    boost_enabled = i;
    // Inititializing the driver before (because if doesn't work we can't do anything).
    if (state_fail(s = driver->init())) {
        return static_dispose(c, s, state_msg);
    }
	// Setting references
	apis_put(ops->init,               mgt_cpufreq_regale_init);
	apis_put(ops->dispose,            mgt_cpufreq_regale_dispose);
	apis_put(ops->count_available,    mgt_cpufreq_regale_count_available);
	apis_put(ops->get_available_list, mgt_cpufreq_regale_get_available_list);
	apis_put(ops->get_current_list,   mgt_cpufreq_regale_get_current_list);
	apis_put(ops->get_nominal,        mgt_cpufreq_regale_get_nominal);
	apis_put(ops->get_index,          mgt_cpufreq_regale_get_index);
	apis_put(ops->set_current_list,   mgt_cpufreq_regale_set_current_list);
	apis_put(ops->set_current,        mgt_cpufreq_regale_set_current);
	apis_put(ops->get_governor,       mgt_cpufreq_regale_governor_get);
	apis_put(ops->get_governor_list,  mgt_cpufreq_regale_governor_get_list);
	apis_put(ops->set_governor,       mgt_cpufreq_regale_governor_set);
	apis_put(ops->set_governor_mask,  mgt_cpufreq_regale_governor_set_mask);
	apis_put(ops->set_governor_list,  mgt_cpufreq_regale_governor_set_list);

	return EAR_SUCCESS;
}

state_t mgt_cpufreq_regale_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_regale_dispose(ctx_t *c)
{
    if (driver != NULL) {
        driver->dispose();
    }
    if (available_list != NULL) {
        free(available_list);
    }
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_regale_count_available(ctx_t *c, uint *pstate_count)
{
	*pstate_count = available_count;
	return EAR_SUCCESS;
}

//
state_t mgt_cpufreq_regale_get_available_list(ctx_t *c, pstate_t *pstate_list)
{
	memcpy(pstate_list, available_list, available_count*sizeof(pstate_t));
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_regale_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
    ullong khz;
    int cpu;

	memset((void *) pstate_list, 0, sizeof(pstate_t)*cpu_count);
    for (cpu = 0; cpu < cpu_count; ++cpu) {
        regale_nm_cpufreq_get_current(cpu, &khz);
        pstate_list[cpu].khz = (ullong) khz;
        get_index(khz, &pstate_list[cpu].idx);
        debug("CPU%d: %llu KHz", cpu, pstate_list[cpu].khz);
    }
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_regale_get_nominal(ctx_t *c, uint *pstate_index)
{
	*pstate_index = idx_nominal;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_regale_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest)
{
    //
}

/** */
state_t mgt_cpufreq_regale_set_current_list(ctx_t *c, uint *pstate_index)
{
    ullong khz;
    int cpu;

    memset((void *) pstate_list, 0, sizeof(pstate_t)*cpu_count);
    for (cpu = 0; cpu < cpu_count; ++cpu) {
        regale_nm_cpufreq_set_current(cpu, pstate_index[cpu]);
        debug("CPU%d: set to %llu KHz", cpu, available_list[cpu].khz);
    }
    return EAR_SUCCESS;
}

/** */
state_t mgt_cpufreq_regale_set_current(ctx_t *c, uint pstate_index, int cpu)
{
    regale_nm_cpufreq_set_current((uint) cpu, pstate_index[cpu]);
    return EAR_SUCCESS;
}

// Governors
static state_t get_governor(int cpu, uint *governor)
{
    regale_nm_governor_get((uint) cpu, governor);
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_regale_governor_get(ctx_t *c, uint *governor)
{
    return get_governor(0, governor);
}

state_t mgt_cpufreq_regale_governor_get_list(ctx_t *c, uint *governors)
{
    int cpu;
    for (cpu = 0; cpu < cpu_count-1; ++cpu) {
        get_governor(cpu, &governors[cpu]);
    }
    return get_governor(cpu, &governors[cpu]);
}

static state_t set_governor(int cpu, uint governor)
{
    regale_nm_governor_set((uint) cpu, governor);
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_regale_governor_set(ctx_t *c, uint governor)
{
    return set_governor(0, governor);
}

state_t mgt_cpufreq_regale_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask)
{
    int cpu;
    for (cpu = 0; cpu < cpu_count; ++cpu) {
        if (CPU_ISSET(cpu, &mask)) {
            set_governor(cpu, governors[cpu]);
        }
    }
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_regale_governor_set_list(ctx_t *c, uint *governors)
{
    int cpu;
    for (cpu = 0; cpu < cpu_count-1; ++cpu) {
        set_governor(cpu, governors[cpu]);
    }
    return set_governor(cpu, governors[cpu]);
}