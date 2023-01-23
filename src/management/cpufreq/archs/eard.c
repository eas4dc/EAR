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

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/utils/serial_buffer.h>
#include <daemon/local_api/eard_api_rpc.h>
#include <management/cpufreq/archs/eard.h>
#include <management/cpufreq/archs/amd17.h>

#define RPC_GET_API          RPC_MGT_CPUFREQ_GET_API
#define RPC_GET_AVAILABLE    RPC_MGT_CPUFREQ_GET_AVAILABLE
#define RPC_GET_CURRENT      RPC_MGT_CPUFREQ_GET_CURRENT
#define RPC_GET_NOMINAL      RPC_MGT_CPUFREQ_GET_NOMINAL
#define RPC_GET_GOVERNOR     RPC_MGT_CPUFREQ_GET_GOVERNOR
#define RPC_SET_CURRENT_LIST RPC_MGT_CPUFREQ_SET_CURRENT_LIST
#define RPC_SET_CURRENT      RPC_MGT_CPUFREQ_SET_CURRENT
#define RPC_SET_GOVERNOR     RPC_MGT_CPUFREQ_SET_GOVERNOR

static uint                 cpu_count;
static ctx_t                driver_c;
static mgt_ps_driver_ops_t *driver;
static pstate_t            *available_list;
static uint                 available_count;
static uint                 boost_enabled;
static uint                 idx_nominal;

state_t mgt_cpufreq_eard_load(topology_t *tp_in, mgt_ps_ops_t *ops, mgt_ps_driver_ops_t *ops_driver, int eard)
{
	uint eard_api = 0;
	state_t s;

	if (!eard) {
		return_msg(EAR_ERROR, "EARD (daemon) not required");
	}
	if (!eards_connected()) {
		return_msg(EAR_ERROR, "EARD (daemon) not connected");
	}
	if (state_fail(s = eard_rpc(RPC_GET_API, NULL, 0, (char *) &eard_api, sizeof(int)))) {
		return s;
	}
	if (eard_api == API_NONE || eard_api == API_DUMMY) {
		return_msg(EAR_ERROR, "EARD (daemon) has loaded DUMMY API");
	}
	// If it is already connected but the API diverges...
	debug("CPUFREQ EARD status passed, received API %u", eard_api);
	cpu_count = tp_in->cpu_count;
	driver = ops_driver;
	// Setting references
	apis_put(ops->init,               mgt_cpufreq_eard_init);
	apis_put(ops->dispose,            mgt_cpufreq_eard_dispose);
	apis_put(ops->count_available,    mgt_cpufreq_eard_count_available);
	apis_put(ops->get_available_list, mgt_cpufreq_eard_get_available_list);
	apis_put(ops->get_current_list,   mgt_cpufreq_eard_get_current_list);
	apis_put(ops->get_nominal,        mgt_cpufreq_eard_get_nominal);
	apis_put(ops->get_index,          mgt_cpufreq_eard_get_index);
	apis_put(ops->set_current_list,   mgt_cpufreq_eard_set_current_list);
	apis_put(ops->set_current,        mgt_cpufreq_eard_set_current);
	apis_put(ops->get_governor,       mgt_cpufreq_eard_governor_get);
	apis_put(ops->get_governor_list,  mgt_cpufreq_eard_governor_get_list);
	apis_put(ops->set_governor,       mgt_cpufreq_eard_governor_set);
	apis_put(ops->set_governor_mask,  mgt_cpufreq_eard_governor_set_mask);
	apis_put(ops->set_governor_list,  mgt_cpufreq_eard_governor_set_list);

	return EAR_SUCCESS;
}

static state_t static_dispose(ctx_t *c, state_t s, char *msg)
{
	if (driver != NULL) {
		driver->dispose(&driver_c);
	}
	if (available_list != NULL) {
		free(available_list);
	}
	return_msg(s, msg);
}

// Init
state_t mgt_cpufreq_eard_init(ctx_t *c)
{
	char *buffer;
	size_t size;
	state_t s;

	// The initialization is just for read values. The init of this API it will
	// be called just if other APIs can't read, because if not these other APIs
	// will offer the following data.
	if (state_fail(s = eard_rpc_buffered(RPC_GET_AVAILABLE, NULL, 0, &buffer, &size))) {
		return s;
	}
	debug("EARD answered with %lu samples in the available list pstate size %u", size / sizeof(pstate_t), sizeof(pstate_t));
	// Inititializing the driver before (because if doesn't work we can't do anything).
	if (state_fail(s = driver->init(&driver_c))) {
		return static_dispose(c, s, state_msg);
	}
	// Filling available P_STATEs
	serial_copy_elem((wide_buffer_t *) &buffer, (char *) &available_count, NULL);
	available_list = calloc(available_count, sizeof(pstate_t));
	serial_copy_elem((wide_buffer_t *) &buffer, (char *) available_list, NULL);
    #if SHOW_DEBUGS
    debug("Available processor frequencies:");
    int i;
    for (i = 0; i < available_count; ++i) {
        debug("PS%d: %llu KHz", i, available_list[i].khz);
    }
    #endif
	// Is boost enabled? We can ask the driver, but EARD may have additional tests.
	if (state_fail(s = eard_rpc(RPC_GET_NOMINAL, NULL, 0, (char *) &idx_nominal, sizeof(uint)))) {
		return static_dispose(c, s, state_msg);
	}
	debug("EARD answered with nominal index %u", idx_nominal);
	boost_enabled = idx_nominal;

	return EAR_SUCCESS;
}

state_t mgt_cpufreq_eard_dispose(ctx_t *c)
{
	return static_dispose(c, EAR_SUCCESS, NULL);
}

state_t mgt_cpufreq_eard_count_available(ctx_t *c, uint *pstate_count)
{
	*pstate_count = available_count;
	return EAR_SUCCESS;
}

//
state_t mgt_cpufreq_eard_get_available_list(ctx_t *c, pstate_t *pstate_list)
{
	memcpy(pstate_list, available_list, available_count*sizeof(pstate_t));
	return EAR_SUCCESS;
}

/** */
state_t mgt_cpufreq_eard_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	memset((void *) pstate_list, 0, sizeof(pstate_t)*cpu_count);
	return eard_rpc(RPC_GET_CURRENT, NULL, 0, (char *) pstate_list, sizeof(pstate_t)*cpu_count);
}

state_t mgt_cpufreq_eard_get_nominal(ctx_t *c, uint *pstate_index)
{
	*pstate_index = idx_nominal;
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_eard_get_index(ctx_t *c, ullong freq_khz, uint *pstate_index, uint closest)
{
	ullong khz;
	ullong aux;
	int pst;

	// Boost test
	if (boost_enabled && available_list[0].khz == freq_khz) {
		*pstate_index = 0;
		return EAR_SUCCESS;
	}
	// Closest test
	if (closest && freq_khz > available_list[idx_nominal].khz) {
		*pstate_index = idx_nominal;
		return EAR_SUCCESS;
	}
	// The searching.
	for (pst = idx_nominal; pst < available_count; ++pst)
	{
		khz = available_list[pst].khz;
		if (freq_khz == khz) {
			*pstate_index = pst;
			return EAR_SUCCESS;
		}
		if (closest && freq_khz > khz) {
			if (pst > idx_nominal) {
				aux = available_list[pst-1].khz - freq_khz;
				khz = freq_khz - khz;
				pst = pst - (aux < khz);
			}
			*pstate_index = pst;
			return EAR_SUCCESS;
		}
	}
	if (closest) {
		*pstate_index = available_count-1;
		return EAR_SUCCESS;
	}
	return_msg(EAR_ERROR, "P_STATE not found");
}

/** */
state_t mgt_cpufreq_eard_set_current_list(ctx_t *c, uint *pstate_index)
{
	return eard_rpc(RPC_SET_CURRENT_LIST, (char *) pstate_index, sizeof(uint)*cpu_count, NULL, 0);
}

/** */
state_t mgt_cpufreq_eard_set_current(ctx_t *c, uint pstate_index, int cpu)
{
	// Defining the sending data
	struct data_s {
		uint pstate_index;
		int cpu;
	} data = {.cpu = cpu, .pstate_index = pstate_index};
	return eard_rpc(RPC_SET_CURRENT, (char *) &data, sizeof(data), NULL, 0);
}

// Governors
state_t mgt_cpufreq_eard_governor_get(ctx_t *c, uint *governor)
{
	state_t s;
	if (state_fail(s = driver->get_governor(&driver_c, governor))) {
		if (state_fail(s = eard_rpc(RPC_GET_GOVERNOR, NULL, 0, (char *) governor, sizeof(uint)))) {
			return s;
		}
	}
	return EAR_SUCCESS;
}

state_t mgt_cpufreq_eard_governor_get_list(ctx_t *c, uint *governors)
{
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_eard_governor_set(ctx_t *c, uint governor)
{
	return eard_rpc(RPC_SET_GOVERNOR, (char *) &governor, sizeof(uint), NULL, 0);
}

state_t mgt_cpufreq_eard_governor_set_mask(ctx_t *c, uint governor, cpu_set_t mask)
{
    return EAR_SUCCESS;
}

state_t mgt_cpufreq_eard_governor_set_list(ctx_t *c, uint *governors)
{
    return EAR_SUCCESS;
}
