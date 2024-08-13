/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1
#include <dlfcn.h>
#include <stdlib.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/system/symplug.h>
#include <metrics/common/nvml.h>
#include <metrics/gpu/archs/nvml.h>

//static nvml_t nvml;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static nvml_t         nvml;
static nvmlDevice_t  *devs;
static suscription_t *sus;
static uint           devs_count;
static gpu_t         *pool;
static uint           ok_pool;
static uint           initialized;

static void load_atfork()
{
    // Just lock control is enough
    pthread_mutex_unlock(&lock);
}

void gpu_nvml_load(gpu_ops_t *ops, int force_api)
{
    debug("Received API %d", force_api);
    if (API_IS(force_api, API_DEFAULT)) {
        ok_pool = 1;
    } else if (API_IS(force_api, API_FREE)) {
        ok_pool = nvml_is_privileged();
    }
    debug("Pooling %d", ok_pool);
    // The locking and duplicate control is at higher level
    if (state_fail(nvml_open(&nvml))) {
        debug("nvml_open failed: %s", state_msg);
        return;
    }
    if (state_fail(nvml_get_devices(&devs, &devs_count))) {
        debug("nvml_get_devices failed: %s", state_msg);
        return;
    }
    // Allocation
    pool = calloc(devs_count, sizeof(gpu_t));
    // Atfork control
    pthread_atfork(NULL, NULL, load_atfork);
    apis_set(ops->get_info,	     gpu_nvml_get_info);
    apis_set(ops->get_devices,   gpu_nvml_get_devices);
    apis_set(ops->init,          gpu_nvml_init);
    apis_set(ops->dispose,       gpu_nvml_dispose);
    apis_pin(ops->read,          gpu_nvml_read, ok_pool);
    apis_set(ops->read_raw,      gpu_nvml_read_raw);
    debug("Loaded NVML");
}

void gpu_nvml_get_info(apinfo_t *info)
{
    info->api         = API_NVML;
    info->devs_count  = devs_count;

    if (nvml.GetArchitecture(devs[0], &info->dev_model) != NVML_SUCCESS) {
        info->dev_model = NVML_DEVICE_ARCH_UNKNOWN;
    }
}

void gpu_nvml_get_devices(gpu_devs_t **devs_in, uint *devs_count_in)
{
    char serial[32];
    nvmlReturn_t r;
    int i;

    if (devs_in != NULL) {
        *devs_in = calloc(devs_count, sizeof(gpu_devs_t));
        //
        for (i = 0; i < devs_count; ++i) {
            if ((r = nvml.GetSerial(devs[i], serial, 32)) != NVML_SUCCESS) {
                return_msg(, nvml.ErrorString(r));
            }
            (*devs_in)[i].serial = (ullong) atoll(serial);
            (*devs_in)[i].index  = i;
        }
    }
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }
}

state_t gpu_nvml_init(ctx_t *c)
{
	state_t s = EAR_SUCCESS;
	timestamp_t time;
    int i;

    if (initialized) {
        return s;
    }
    // Initializing pool
	timestamp_getfast(&time);
	for (i = 0; i < devs_count; ++i) {
		pool[i].time    = time;
        pool[i].correct = 1;
    }
    if (ok_pool) {
	// Initializing monitoring thread suscription.
	sus = suscription();
	sus->call_main  = gpu_nvml_pool;
	sus->time_relax = 2000;
	sus->time_burst = 1000;
	// Initializing monitoring thread.
	if (state_ok(s = sus->suscribe(sus))) {
		initialized = 1;
	}
    } // ok_pool
	return s;
}

state_t gpu_nvml_dispose(ctx_t *c)
{
    if (initialized) {
        monitor_unregister(sus);
        initialized = 0;
    }
	return EAR_SUCCESS;
}

static int static_read(int i, gpu_t *metric)
{
	nvmlEnableState_t mode;
	int s;

	// Cleaning
	memset(metric, 0, sizeof(gpu_t));
	// Testing if all is right
	if ((s = nvml.GetPowerMode(devs[i], &mode)) != NVML_SUCCESS) {
		return 0;
	}
	if (mode != NVML_FEATURE_ENABLED) {
		return 0;
	}
	nvmlProcessInfo_t procs[8];
	nvmlUtilization_t util;
	uint proc_count = 8;
	uint freq_gpu_mhz;
	uint freq_mem_mhz;
	uint temp_gpu;
	uint power_mw;
	// Getting the metrics by calling NVML (no MEM temp). It can be 
	// passed 1, 2, 20 seconds, and so on... And the NVML calls will
	// return the same values. It is supposed to return the values
	// of the last second. But there is no problem because its is
	// divided by the number of samples. 
	s = nvml.GetPowerUsage(devs[i], &power_mw);
	s = nvml.GetClocks(devs[i], NVML_CLOCK_MEM, &freq_mem_mhz);
	s = nvml.GetClocks(devs[i], NVML_CLOCK_SM , &freq_gpu_mhz);
	s = nvml.GetTemp(devs[i], NVML_TEMPERATURE_GPU, &temp_gpu);
	s = nvml.GetUtil(devs[i], &util);
	s = nvml.GetProcs(devs[i], &proc_count, procs);
    debug("D%d returned: %u W, %u MHz, %u º",
        i, power_mw / 1000, freq_gpu_mhz, temp_gpu);
	// Pooling the data (time is not set here)
	metric->samples  = 1;
	metric->freq_mem = (ulong) freq_mem_mhz * 1000LU;
	metric->freq_gpu = (ulong) freq_gpu_mhz * 1000LU;
	metric->util_mem = (ulong) util.memory;
	metric->util_gpu = (ulong) util.gpu;
	metric->temp_gpu = (ulong) temp_gpu;
	metric->temp_mem = 0;
	metric->power_w  = ((double) power_mw) / 1000.0;
	metric->energy_j = 0;
	metric->working  = proc_count > 0;
    // In the future we have to check if there is any error in any returned status.
	metric->correct  = 1;

	return 1;
}

state_t gpu_nvml_pool(void *p)
{
	timestamp_t time;
    double time_diff;
	gpu_t metric;
	int i;

    debug("Pooling");
	//
	uint working = 0;
	// Lock
	while (pthread_mutex_trylock(&lock));
    // Time operations
	timestamp_getfast(&time);
    time_diff = (double) timestamp_diff(&time, &pool[0].time, TIME_USECS);
    time_diff = time_diff / 1000000.0;
	//
	for (i = 0; i < devs_count; ++i) {
		if (!static_read(i, &metric)) {
			continue;
		}
		// Pooling the data
		pool[i].time      = time;
		pool[i].samples  += metric.samples;
		pool[i].freq_mem += metric.freq_mem;
		pool[i].freq_gpu += metric.freq_gpu;
		pool[i].util_mem += metric.util_mem;
		pool[i].util_gpu += metric.util_gpu;
		pool[i].temp_gpu += metric.temp_gpu;
		pool[i].temp_mem += metric.temp_mem;
		pool[i].power_w  += metric.power_w;
		pool[i].energy_j += metric.power_w * time_diff;
		pool[i].working   = metric.working;
		pool[i].correct   = metric.correct;
		// Burst or not
		working += pool[i].working;
	}
	// Lock
	pthread_mutex_unlock(&lock);
	
    #if 0
	if (working  > 0 && !monitor_is_bursting(sus)) {
		debug("bursting");
		monitor_burst(sus, 0);
	}
	if (working == 0 &&  monitor_is_bursting(sus)) {
		debug("relaxing");
		monitor_relax(sus);
	}
    #endif

	return EAR_SUCCESS;
}

state_t gpu_nvml_read(ctx_t *c, gpu_t *data)
{
    // Updating pool
    gpu_nvml_pool(NULL);
	while (pthread_mutex_trylock(&lock));
	memcpy(data, pool, devs_count * sizeof(gpu_t));
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t gpu_nvml_read_raw(ctx_t *c, gpu_t *data)
{
	timestamp_t time;
	int i;
	timestamp_getfast(&time);
	for (i = 0; i < devs_count; ++i) {
		static_read(i, &data[i]);
		data[i].time     = time;
		data[i].power_w /= 1000;
	}
	return EAR_SUCCESS;
}
