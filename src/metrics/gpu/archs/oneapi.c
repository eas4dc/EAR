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

// #define SHOW_DEBUGS 1

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/system/symplug.h>
#include <metrics/common/oneapi.h>
#include <metrics/gpu/archs/oneapi.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static gpu_t          *pool; // change to sysmans_pool
static timestamp_t     pool_time1;
static timestamp_t     pool_time2;
static suscription_t  *sus;
static zes_device_handle_t *devs;
static uint            devs_count;
static ze_t            ze;

#define ret_fail(function) \
    if ((z = function) != ZE_RESULT_SUCCESS) { \
        debug("Failed " #function " with error 0x%x", z); \
        return EAR_ERROR; \
    }

void gpu_oneapi_load(gpu_ops_t *ops)
{
    if (state_fail(oneapi_open(&ze))) {
        debug("oneapi_open failed: %s", state_msg);
        return;
    }
    if (state_fail(oneapi_get_devices(&devs, &devs_count))) {
        debug("oneapi_get_devices failed: %s", state_msg);
        return;
    }
    apis_set(ops->get_api,       gpu_oneapi_get_api);
    apis_set(ops->init,          gpu_oneapi_init);
    apis_set(ops->dispose,       gpu_oneapi_dispose);
    apis_set(ops->count_devices, gpu_oneapi_count_devices);
    apis_set(ops->read,          gpu_oneapi_read);
    apis_set(ops->read_raw,      gpu_oneapi_read_raw);
}

void gpu_oneapi_get_api(uint *api)
{
    *api = API_ONEAPI;
}

state_t gpu_oneapi_init(ctx_t *c)
{
    state_t s;
    #if 1
	// Monitor suscription
	pool = calloc(devs_count, sizeof(gpu_t));
	// Initializing monitoring thread suscription.
	sus = suscription();
	sus->call_main  = gpu_oneapi_pool;
	sus->time_relax = 2000;
	sus->time_burst = 1000;
	// Initializing monitoring thread.
	if (state_fail(s = sus->suscribe(sus))) {
		debug("GPU monitor FAILS");
		monitor_unregister(sus);
		free(pool);
	}
    #endif
	return EAR_SUCCESS;
}

state_t gpu_oneapi_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t gpu_oneapi_count_devices(ctx_t *c, uint *dev_count)
{
	*dev_count = devs_count;
	return EAR_SUCCESS;
}

static state_t oneapi_read_power(int dv, gpu_t *data)
{
	zes_pwr_handle_t           domains[4]; // Its enough
	uint                       domains_count = 4;
	//zes_power_properties_t     props;
	zes_power_energy_counter_t info;
	uint                       ma;
	ze_result_t                z;

	// Retrieving the domains
	ret_fail(ze.sDeviceEnumPowerDomains(devs[dv], &domains_count, domains));
	
	for (ma = 0; ma < domains_count; ++ma) {
		ret_fail(ze.sPowerGetEnergyCounter(domains[ma], &info));
		// It has to be tested.
		data->energy_j = ((double) info.energy) / 1000000.0;
	}

	return EAR_SUCCESS;
}

static state_t oneapi_read_utilization(int dv, gpu_t *data)
{
	zes_engine_handle_t     domains[4]; // Its enough
	uint                    domains_count = 4;
	zes_engine_properties_t props;
	zes_engine_stats_t      info;
	uint                    ma;
	ze_result_t             z;

	// Retrieving the domains
	ret_fail(ze.sDeviceEnumEngineGroups(devs[dv], &domains_count, domains));
	
	for (ma = 0; ma < domains_count; ++ma) {
		// Getting domain characteristics
		ret_fail(ze.sEngineGetProperties(domains[ma], &props));
		// Maybe ZES_ENGINE_GROUP_COMPUTE_ALL?
		if (props.type == ZES_ENGINE_GROUP_ALL) {
			ret_fail(ze.sEngineGetActivity(domains[ma], &info));
			// Utilization here, it has to be tested
			data->util_gpu = (ulong) (info.activeTime); // In microseconds
		}
	}

	return EAR_SUCCESS;
}

static state_t oneapi_read_frequency(int dv, gpu_t *data)
{
	zes_freq_handle_t     domains[4]; // Its enough
	uint                  domains_count;
	zes_freq_properties_t props;
	zes_freq_state_t      info;
	uint                  ma;
	ze_result_t           z;

	// Retrieving the domains
	ret_fail(ze.sDeviceEnumFrequencyDomains(devs[dv], &domains_count, domains));
	for (ma = 0; ma < domains_count; ++ma) {
		// Getting domain characteristics
		ret_fail(ze.sFrequencyGetProperties(domains[ma], &props));
		//
		ret_fail(ze.sFrequencyGetState(domains[ma], &info));
		// If is not GPU is MEMORY and we need both
		if (props.type == ZES_FREQ_DOMAIN_GPU) {
			data->freq_gpu = (ulong) info.actual; // MHz
		}
		if (props.type == ZES_FREQ_DOMAIN_MEMORY) {
			data->freq_mem = (ulong) info.actual; // MHz
		}
	}

	return EAR_SUCCESS;
}

static state_t oneapi_read_temperature(int dv, gpu_t *data)
{
	//zes_temp_handle_t domains[6]; // Its enough
	// Future.
    return EAR_SUCCESS;
}

static void read_single(int dv, gpu_t *gpu)
{
	// Cleanin
	memset(gpu, 0, sizeof(gpu_t));
	// Reading
	oneapi_read_power(dv, gpu);
	oneapi_read_frequency(dv, gpu);
	oneapi_read_utilization(dv, gpu);
	oneapi_read_temperature(dv, gpu);
}

state_t gpu_oneapi_pool(void *p)
{
	gpu_t       current;
	int         dv;

	// Locking
	while (pthread_mutex_trylock(&lock));
	// Getting current time
	timestamp_getfast(&pool_time2);
	// Adding to the pool
	for (dv = 0; dv < devs_count; ++dv)
	{
		read_single(dv, &current);

		pool[dv].time      = pool_time2;
		pool[dv].samples  += 1;
		pool[dv].freq_mem += current.freq_mem;
		pool[dv].freq_gpu += current.freq_gpu;
		pool[dv].util_mem += 0LU;
		pool[dv].util_gpu += current.util_gpu;
		pool[dv].temp_gpu += current.temp_gpu;
		pool[dv].temp_mem += current.temp_mem;
		pool[dv].energy_j += current.energy_j;
		pool[dv].power_w  += 0.0;
		pool[dv].working   = current.working;
		pool[dv].correct   = current.correct;
		
		debug("dev | metric   | value");
		debug("--- | -------- | -----");
		debug("%d   | freq     | %lu MHz", dv, current.freq_gpu);
		debug("%d   | energy   | %0.2lf J", dv, current.energy_j);
		debug("%d   | util     | %lu %%", dv, current.util_gpu);
	}
	// Copying time
	pool_time1 = pool_time2;
	// Unlock
	pthread_mutex_unlock(&lock);

	return EAR_SUCCESS;
}

state_t gpu_oneapi_read(ctx_t *c, gpu_t *data)
{
	while (pthread_mutex_trylock(&lock));
	memcpy(data, pool, devs_count * sizeof(gpu_t));
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t gpu_oneapi_read_raw(ctx_t *c, gpu_t *data)
{
	// First it requires tests
	return EAR_SUCCESS;
}
