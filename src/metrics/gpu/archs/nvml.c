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

#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/system/symplug.h>
#include <dlfcn.h>
#include <metrics/common/nvml.h>
#include <metrics/gpu/archs/nvml.h>
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static nvml_t nvml;
static nvmlDevice_t *handlers;
static uint handlers_count;
static suscription_t *sus;
static gpu_t *pool;
static uint is_pooling;

static void load_atfork()
{
    // Just lock control is enough
    pthread_mutex_unlock(&lock);
}

static void close_all()
{
    if (is_pooling) {
        monitor_unregister(sus);
        is_pooling = 0;
    }
    if (pool != NULL) {
        free(pool);
        pool = NULL;
    }
    if (handlers != NULL) {
        free(handlers);
        handlers = NULL;
    }
    nvml_close();
}

GPU_F_LOAD(nvml)
{
    timestamp_t time;
    int i;

    debug("Received API %d", options);
    if (api_already_loaded(ops)) {
        return;
    }
    if (API_IS(options, API_DEFAULT)) {
        is_pooling = 1;
    } else if (API_IS(options, API_FREE)) {
        is_pooling = nvml_is_privileged();
    }
    debug("Pooling %d", is_pooling);
    // The locking and duplicate control is at higher level
    if (state_fail(nvml_open(&nvml))) {
        debug("nvml_open failed: %s", state_msg);
        return;
    }
    // Allocation
    nvml_get_handlers(&handlers, &handlers_count);
    pool = calloc(handlers_count, sizeof(gpu_t));
    debug("Detected handlers: %d", handlers_count);
    // Atfork control
    pthread_atfork(NULL, NULL, load_atfork);
    // Initializing pool
    timestamp_getfast(&time);
    for (i = 0; i < handlers_count; ++i) {
        pool[i].time    = time;
        pool[i].correct = 1;
    }
    if (is_pooling) {
        sus             = suscription();
        sus->call_main  = gpu_nvml_pool;
        sus->time_relax = 2000;
        sus->time_burst = 1000;
        sus->suscribe(sus);
    }
    //
    apis_pif(ops->unload, gpu_nvml_unload, 1);
    apis_pif(ops->get_info, gpu_nvml_get_info, 1);
    apis_pif(ops->topology_get, gpu_nvml_topology_get, 1);
    apis_pif(ops->read, gpu_nvml_read, is_pooling);
    apis_pif(ops->read_raw, gpu_nvml_read_raw, 1);
    debug("Loaded NVML");
}

GPU_F_UNLOAD(nvml)
{
    close_all();
}

GPU_F_GET_INFO(nvml)
{
    nvmlReturn_t ret;

    info->api        = API_NVML;
    info->devs_count = handlers_count;
    if ((ret = nvml.GetArchitecture(handlers[0], &info->dev_model)) != NVML_SUCCESS) {
        debug("nvml GetArchitecture returned: %d", ret);
        info->dev_model = NVML_DEVICE_ARCH_UNKNOWN;
    }
}

GPU_F_TOPOLOGY_GET(nvml)
{
    nvml_get_devices(&tp->devs, &tp->devs_count, 1);
}

static int static_read(int i, gpu_t *metric)
{
    nvmlEnableState_t mode;
    int s;

    // Cleaning
    memset(metric, 0, sizeof(gpu_t));
    // Testing if all is right
    if ((s = nvml.GetPowerMode(handlers[i], &mode)) != NVML_SUCCESS) {
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
    s = nvml.GetPowerUsage(handlers[i], &power_mw);
    s = nvml.GetClocks(handlers[i], NVML_CLOCK_MEM, &freq_mem_mhz);
    s = nvml.GetClocks(handlers[i], NVML_CLOCK_SM, &freq_gpu_mhz);
    s = nvml.GetTemp(handlers[i], NVML_TEMPERATURE_GPU, &temp_gpu);
    s = nvml.GetUtil(handlers[i], &util);
    s = nvml.GetProcs(handlers[i], &proc_count, procs);
    debug("D%d returned: %u W, %u MHz, %u º", i, power_mw / 1000, freq_gpu_mhz, temp_gpu);
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
    metric->correct = 1;

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
#if SHOW_DEBUGS
    uint working = 0;
#endif
    // Lock
    while (pthread_mutex_trylock(&lock))
        ;
    // Time operations
    timestamp_getfast(&time);
    time_diff = (double) timestamp_diff(&time, &pool[0].time, TIME_USECS);
    time_diff = time_diff / 1000000.0;
    //
    for (i = 0; i < handlers_count; ++i) {
        if (!static_read(i, &metric)) {
            continue;
        }
        // Pooling the data
        pool[i].time = time;
        pool[i].samples += metric.samples;
        pool[i].freq_mem += metric.freq_mem;
        pool[i].freq_gpu += metric.freq_gpu;
        pool[i].util_mem += metric.util_mem;
        pool[i].util_gpu += metric.util_gpu;
        pool[i].temp_gpu += metric.temp_gpu;
        pool[i].temp_mem += metric.temp_mem;
        pool[i].power_w += metric.power_w;
        pool[i].energy_j += metric.power_w * time_diff;
        pool[i].working = metric.working;
        pool[i].correct = metric.correct;
        // Burst or not
#if SHOW_DEBUGS
        working += pool[i].working;
#endif
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

GPU_F_READ(nvml)
{
    // Updating pool
    gpu_nvml_pool(NULL);
    while (pthread_mutex_trylock(&lock))
        ;
    memcpy(d, pool, handlers_count * sizeof(gpu_t));
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

GPU_F_READ_RAW(nvml)
{
    timestamp_t time;
    int i;

    timestamp_getfast(&time);
    for (i = 0; i < handlers_count; ++i) {
        static_read(i, &d[i]);
        d[i].time = time;
        d[i].power_w /= 1000;
    }
    return EAR_SUCCESS;
}