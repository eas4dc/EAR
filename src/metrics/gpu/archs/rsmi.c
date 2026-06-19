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
#include <metrics/common/rsmi.h>
#include <metrics/gpu/archs/rsmi.h>
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static rsmi_t rsmi;
static suscription_t *sus;
static uint handlers_count;
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
    rsmi_close();
}

GPU_F_LOAD(rsmi)
{
    timestamp_t time;
    int i;

    if (API_IS(options, API_DEFAULT)) {
        is_pooling = 1;
    } else if (API_IS(options, API_FREE)) {
        is_pooling = rsmi_is_privileged();
    }
    debug("Pooling %d", is_pooling);
    if (state_fail(rsmi_open(&rsmi))) {
        debug("rsmi_open failed: %s", state_msg);
        return;
    }
    if (rsmi_fail(rsmi.devs_count(&handlers_count))) {
        debug("rsmi.devs_count failed");
        return;
    }
    // Allocation
    pool = calloc(handlers_count, sizeof(gpu_t));
    // Atfork control
    pthread_atfork(NULL, NULL, load_atfork);
    // Initializing pool (pool at 0 is not incorrect)
    timestamp_getfast(&time);
    for (i = 0; i < handlers_count; ++i) {
        pool[i].time    = time;
        pool[i].correct = 1;
    }
    if (is_pooling) {
        // Initializing monitoring thread suscription.
        sus             = suscription();
        sus->call_main  = gpu_rsmi_pool;
        sus->time_relax = 2000;
        sus->time_burst = 1000;
        sus->suscribe(sus);
    }
    apis_pif(ops->unload, gpu_rsmi_unload, 1);
    apis_pif(ops->get_info, gpu_rsmi_get_info, 1);
    apis_pif(ops->topology_get, gpu_rsmi_topology_get, 1);
    apis_pif(ops->read, gpu_rsmi_read, is_pooling);
    apis_pif(ops->read_raw, gpu_rsmi_read_raw, 1);
    debug("Loaded RSMI");
}

GPU_F_UNLOAD(rsmi)
{
    close_all();
}

GPU_F_GET_INFO(rsmi)
{
    info->api        = API_RSMI;
    info->devs_count = handlers_count;
}

GPU_F_TOPOLOGY_GET(rsmi)
{
    rsmi_get_devices(&tp->devs, &tp->devs_count);
}

static int is_working(int i)
{
    rsmi_process_info_t processes[32];
    uint processes_count = 32;
    uint devices_count   = 32;
    uint devices[32];
    int p, d;

    // Getting the list of total processes
    rsmi.get_procs(processes, &processes_count);
    // Iterating over processes
    for (p = 0; p < processes_count; ++p) {
        // Getting list of devices used by the process p
        rsmi.get_procs_devs(processes[p].process_id, devices, &devices_count);
        // Iterating over devices
        for (d = 0; d < devices_count; ++d) {
            if (devices[d] == i) {
                return 1;
            }
        }
    }
    return 0;
}

static int static_read(int i, gpu_t *metric)
{
    rsmi_freqs_t gpu_hz;
    rsmi_freqs_t mem_hz;
    ullong power_uw = 0;
    ullong temp_mc  = 0;
    rsmi_status_t s;
    uint gpu_util;
    uint mem_util;

    // Cleaning
    memset(metric, 0, sizeof(gpu_t));
    // Testing if all is right
    s = rsmi.get_power(i, 0, &power_uw);
    s = rsmi.get_temperature(i, RSMI_TEMP_TYPE_EDGE, RSMI_TEMP_CURRENT, &temp_mc);
    s = rsmi.get_clock(i, RSMI_CLK_TYPE_SYS, &gpu_hz);
    s = rsmi.get_clock(i, RSMI_CLK_TYPE_MEM, &mem_hz);
    s = rsmi.get_busy_gpu(i, &gpu_util);
    s = rsmi.get_busy_mem(i, &mem_util);
#if 0
    int f;
    debug("D%d GPU temp  : %llu mC", i, temp_mc)
    debug("D%d GPU power : %llu uW", i, power_uw)
    debug("D%d GPU clocks: %d supported, %d current", i, gpu_hz.num_supported, gpu_hz.current);
    debug("D%d MEM clocks: %d supported, %d current", i, mem_hz.num_supported, mem_hz.current);
    for (f = 0; f < gpu_hz.num_supported; ++f) debug("D%d GPU F%d: %lu", i, f, gpu_hz.frequency[f]);
    for (f = 0; f < mem_hz.num_supported; ++f) debug("D%d MEM F%d: %lu", i, f, mem_hz.frequency[f]);
#endif
    // Pooling the data (time is not set here)
    metric->samples  = 1;
    metric->freq_mem = ((ulong) mem_hz.frequency[mem_hz.current]) / 1000LU;
    metric->freq_gpu = ((ulong) gpu_hz.frequency[gpu_hz.current]) / 1000LU;
    metric->util_mem = (ulong) mem_util;
    metric->util_gpu = (ulong) gpu_util;
    metric->temp_gpu = (ulong) temp_mc / 1000LU;
    metric->temp_mem = 0;
    metric->power_w  = ((double) power_uw) / 1000000.0;
    metric->energy_j = 0;
    metric->working  = is_working(i);
    // In the future we have to check if there is any error in any returned status.
    metric->correct = 1;
    //
    unused(s);
    return 1;
}

state_t gpu_rsmi_pool(void *p)
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
    return EAR_SUCCESS;
}

GPU_F_READ(rsmi)
{
    // Updating pool
    gpu_rsmi_pool(NULL);
    while (pthread_mutex_trylock(&lock))
        ;
    memcpy(d, pool, handlers_count * sizeof(gpu_t));
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

GPU_F_READ_RAW(rsmi)
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
