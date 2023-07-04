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
static uint            devs_count;
static ze_handlers_t  *devs;
static ze_t            ze;

#define ret_fail(function) \
    if ((z = function) != ZE_RESULT_SUCCESS) { \
        debug("Failed " #function ": %s (0x%x)", oneapi_strerror(z), z); \
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
    zes_pwr_handle_t           cards[32];
	zes_power_energy_counter_t info;
	uint                       d;
	ze_result_t                z;

	// Retrieving the domains
	for (d = 0; d < devs[dv].spowers_count; ++d) {
        debug("-- DEVICE%d POWER DOMAIN%d ---------------", dv, d);
        debug("canControl: %d", devs[dv].spowers_props[d].canControl)
        debug("onSubdevice: 0x%x", devs[dv].spowers_props[d].onSubdevice)
        debug("defaultLimit: %d", devs[dv].spowers_props[d].defaultLimit)
        debug("minLimit: %d", devs[dv].spowers_props[d].minLimit)
        debug("maxLimit: %d", devs[dv].spowers_props[d].maxLimit)
		ret_fail(ze.sPowerGetEnergyCounter(devs[dv].spowers[d], &info));
        debug("energy: %lu mJ", info.energy);
        debug("timestamp: %lu us", info.timestamp);
        debug("----------------------------------------");
		// It has to be tested.
		data->energy_j = ((double) info.energy) / 1000000.0;
	}
    ret_fail(ze.sDeviceGetCardPowerDomain(devs[dv].sdevice, &cards[0]));
    ret_fail(ze.sPowerGetEnergyCounter(cards[0], &info));
    debug("-- DEVICE%d POWER CARD%d ---------------", dv, 0);
    debug("card0.energy: %lu mJ", info.energy);
    debug("card0.timestamp: %lu us", info.timestamp);
    debug("----------------------------------------");

	return EAR_SUCCESS;
}

static state_t oneapi_read_frequency(int dv, gpu_t *data)
{
    zes_freq_state_t      info;
    uint                  d;
    ze_result_t           z;

    // Retrieving the domains
    for (d = 0; d < devs[dv].sfreqs_count; ++d) {
        debug("-- DEVICE%d FREQ DOMAIN%d ----------------", dv, d);
        debug("canControl: %d", devs[dv].sfreqs_props[d].canControl)
        debug("onSubdevice: %d", devs[dv].sfreqs_props[d].onSubdevice)
        debug("subdeviceId: 0x%x", devs[dv].sfreqs_props[d].subdeviceId)
        debug("min: %lf", devs[dv].sfreqs_props[d].min)
        debug("max: %lf", devs[dv].sfreqs_props[d].max)
        ret_fail(ze.sFrequencyGetState(devs[dv].sfreqs[d], &info));
        debug("actual: %lu MHz", (ulong) info.actual)
        if (devs[dv].sfreqs_props[d].type == ZES_FREQ_DOMAIN_GPU) {
            data->freq_gpu = (ulong) info.actual; // MHz
        }
        if (devs[dv].sfreqs_props[d].type == ZES_FREQ_DOMAIN_MEMORY) {
            data->freq_mem = (ulong) info.actual; // MHz
        }
        debug("----------------------------------------");
    }
    return EAR_SUCCESS;
}

static state_t oneapi_read_utilization(int dv, gpu_t *data)
{
	zes_engine_stats_t      info;
	uint                    g;
	ze_result_t             z;

	for (g = 0; g < devs[dv].sengines_count; ++g) {
        debug("-- DEVICE%d ENGINES DOMAIN%d -------------", dv, g);
        debug("type: %d", devs[dv].sengines_props[g].type)
        ret_fail(ze.sEngineGetActivity(devs[dv].sengines[g], &info));
        debug("activeTime: %lu", info.activeTime);
        debug("timestamp: %lu", info.timestamp);
		if (devs[dv].sengines_props[g].type == ZES_ENGINE_GROUP_ALL) {
			data->util_gpu = (ulong) (info.activeTime / info.timestamp); // In microseconds
		}
        debug("----------------------------------------");
	}
	return EAR_SUCCESS;
}

static state_t oneapi_read_temperature(int dv, gpu_t *data)
{
    zes_psu_state_t       state;
    double                temp;
    uint                  s;
    ze_result_t           z;

    for (s = 0; s < devs[dv].stemps_count; ++s) {
        debug("-- DEVICE%d TEMPERATURE DOMAIN%d ---------", dv, s);
        debug("type: %d", devs[dv].stemps_props[s].type);
        debug("onSubdevice: %d", devs[dv].stemps_props[s].onSubdevice);
        debug("subdeviceId: %d", devs[dv].stemps_props[s].subdeviceId);
        debug("maxTemperature: %lf", devs[dv].stemps_props[s].maxTemperature);
        ret_fail(ze.sTemperatureGetState(devs[dv].stemps[s], &temp));
        if (devs[dv].stemps_props[s].type != ZES_TEMP_SENSORS_GPU) {
            data->temp_gpu = (ulong) temp;
        }
        if (devs[dv].stemps_props[s].type != ZES_TEMP_SENSORS_MEMORY) {
            data->temp_mem = (ulong) temp;
        }
        debug("----------------------------------------");
    }
    for (s = 0; s < devs[dv].spsus_count; ++s) {
        debug("-- DEVICE%d PSUS DOMAIN%d --------------", dv, s);
        ret_fail(ze.sPsuGetState(devs[dv].spsus[s], &state));
        debug("temperature: %d º", state.temperature);
        debug("----------------------------------------");
    }

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
		
		debug("dev | metric   | value    ");
		debug("--- | -------- | -----    ");
		debug("%d   | freq     | %lu MHz ", dv, current.freq_gpu);
		debug("%d   | energy   | %0.2lf J", dv, current.energy_j);
		debug("%d   | util     | %lu %%  ", dv, current.util_gpu);
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
    gpu_oneapi_pool(NULL);
	return EAR_SUCCESS;
}