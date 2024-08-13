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
#include <unistd.h>
#include <pthread.h>
#include <common/sizes.h>
#include <common/plugins.h>
#include <common/output/debug.h>
#include <management/cpufreq/cpufreq.h>
#include <management/cpufreq/archs/eard.h>
#include <management/cpufreq/archs/dummy.h>
#include <management/cpufreq/archs/amd17.h>
#include <management/cpufreq/archs/default.h>
#include <management/cpufreq/drivers/intel_pstate.h>
#include <management/cpufreq/drivers/acpi_cpufreq.h>

static pthread_mutex_t     lock = PTHREAD_MUTEX_INITIALIZER;
static mgt_ps_driver_ops_t ops_driver;
       mgt_ps_ops_t        ops;
static uint                cpu_count;
static uint                api;

state_t mgt_cpufreq_load(topology_t *tp, int eard)
{
    state_t s;
    while (pthread_mutex_trylock(&lock));
    if (api != API_NONE) {
        pthread_mutex_unlock(&lock);
        return EAR_SUCCESS;
    }
    cpu_count = tp->cpu_count;
    // Driver API load
    mgt_acpi_cpufreq_load(tp, &ops_driver);
    mgt_intel_pstate_load(tp, &ops_driver);
	// API load
	api = API_DUMMY;
	// AMD17 loads if MSR test is passed and driver can be initialized. But it
	// does not add its set functions if driver can't write. Also, AMD17 goes
	// before because it works with DEFAULT but the AMD17 dedicated API is
	// more powerful.
	if (state_ok(s = mgt_cpufreq_amd17_load(tp, &ops, &ops_driver))) {
		api = API_AMD17;
		debug("Loaded AMD17");
	}
	// Default loads if driver can be initialized. But it does not add its set
	// frequency and governor functions if driver can't write.
	if (state_ok(s = mgt_cpufreq_default_load(tp, &ops, &ops_driver))) {
		api = API_DEFAULT;
		debug("Loaded DEFAULT");
	}
	// EARD loads if daemon is present and loaded an effective API.
	//
	// But, what happens if is AMD17, but fails, and it enters in default and
	// in default is OK?
	if (state_ok(s = mgt_cpufreq_eard_load(tp, &ops, &ops_driver, eard))) {
		api = API_EARD;
		debug("Loaded EARD");
	}
	// DUMMY loads always
	mgt_cpufreq_dummy_load(tp, &ops);
	debug("Loaded DUMMY");
    // PRIORITY subsystem
    mgt_cpufreq_prio_load(tp, eard);
		debug("API %d", api);
    // Finishing
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

void mgt_cpufreq_get_api(uint *api)
{
    apinfo_t info;
    mgt_cpufreq_get_info(&info);
    *api = info.api;
}

void mgt_cpufreq_get_info(apinfo_t *info)
{
    info->layer       = "CPUFREQ";
    info->api         = API_NONE;
    info->devs_count  = 0;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_THREAD;

    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

void mgt_cpufreq_count_devices(ctx_t *c, uint *devs_count)
{
    apinfo_t info;
    mgt_cpufreq_get_info(&info);
    *devs_count = info.devs_count;
}

state_t mgt_cpufreq_init(ctx_t *c)
{
	state_t s;
    if (state_fail(s = ops.init(c))) {
        return s;
    }
    return mgt_cpufreq_prio_init();
}

state_t mgt_cpufreq_dispose(ctx_t *c)
{
	state_t s;
    if (state_fail(s = ops.dispose(c))) {
        return s;
    }
    return mgt_cpufreq_prio_dispose();
}

void mgt_cpufreq_get_freq_details(freq_details_t *details)
{
    if (ops.get_freq_details != NULL) {
        ops.get_freq_details(details);
    }
}

state_t mgt_cpufreq_get_available_list(ctx_t *c, const pstate_t **pstate_list, uint *pstate_count)
{
	static pstate_t *list = NULL;
	static uint count;
	state_t s;

	if (list == NULL) {
		// Getting the total available P_STATEs
		if (state_fail(s = ops.count_available(c, &count))) {
			return s;
		}
		list = calloc(count, sizeof(pstate_t));
	}
	*pstate_list = list;
	if (pstate_count != NULL) {
		*pstate_count = count;
	}
	preturn (ops.get_available_list, c, list);
}

state_t mgt_cpufreq_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	preturn (ops.get_current_list, c, pstate_list);
}

state_t mgt_cpufreq_get_nominal(ctx_t *c, uint *pstate_index)
{
	preturn (ops.get_nominal, c, pstate_index);
}

state_t mgt_cpufreq_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest)
{
	preturn (ops.get_index, c, freq_khz, pstate_index, closest);
}

/** Setters */
state_t mgt_cpufreq_set_current_list(ctx_t *c, uint *pstate_index)
{
	preturn (ops.set_current_list, c, pstate_index);
}

state_t mgt_cpufreq_set_current(ctx_t *c, uint pstate_index, int cpu)
{
	preturn (ops.set_current, c, pstate_index, cpu);
}

state_t mgt_cpufreq_reset(ctx_t *c)
{
	preturn (ops.reset, c);
}

/** Governors */
state_t mgt_cpufreq_governor_get(ctx_t *c, uint *governor)
{
    preturn (ops.get_governor, c, governor);
}

state_t mgt_cpufreq_governor_get_list(ctx_t *c, uint *governors)
{
    preturn (ops.get_governor_list, c, governors);
}

state_t mgt_cpufreq_governor_set(ctx_t *c, uint governor)
{
    preturn (ops.set_governor, c, governor);
}

state_t mgt_cpufreq_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask)
{
    preturn (ops.set_governor_mask, c, governor, mask);
}

state_t mgt_cpufreq_governor_set_list(ctx_t *c, uint *governors)
{
    preturn (ops.set_governor_list, c, governors);
}

int mgt_cpufreq_governor_is_available(ctx_t *c, uint governor)
{
    return ops_driver.is_governor_available(governor);
}


/** Data */
void mgt_cpufreq_data_alloc(pstate_t **pstate_list, uint **index_list)
{
	if (pstate_list) {
		*pstate_list = calloc(cpu_count, sizeof(pstate_t));
	}
	if (index_list) {
		*index_list = calloc(cpu_count, sizeof(uint));
	}
}

void mgt_cpufreq_data_print(pstate_t *ps_list, uint ps_count, int fd)
{
	pstate_print(ps_list, ps_count, fd);
}

char *mgt_cpufreq_data_tostr(pstate_t *ps_list, uint ps_count, char *buffer, int length)
{
	return pstate_tostr(ps_list, ps_count, buffer, length);
}
