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
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <metrics/common/rsmi.h>
#include <metrics/gpu/archs/rsmi.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static rsmi_t          rsmi;
static suscription_t  *sus;
static uint            devs_count;
static gpu_t          *pool;
static uint            init;
static uint            ok_pool;

void gpu_rsmi_load(gpu_ops_t *ops, int eard)
{
    // If EARD is demanded we do not want pooling.
    // If EARD is not demanded but RSMI is unprivileged
    // means that this function is called by the library.
    #if 0
    ok_pool = (eard == 0) && (rsmi_is_privileged() == 1);
    ok_pool = 1;
    #endif
    if (eard == 2)      ok_pool = 1;
    else if (eard == 1) ok_pool = 0;
    else if (eard == 0){
      if (rsmi_is_privileged() == 1) ok_pool = 1;
      else                      ok_pool = 0;
    }

    debug("Pooling %d", ok_pool);
    if (state_fail(rsmi_open(&rsmi))) {
        debug("rsmi_open failed: %s", state_msg);
        return;
    }
    if (rsmi_fail(rsmi.devs_count(&devs_count))) {
        debug("rsmi.devs_count failed");
        return;
    }
    apis_set(ops->get_api,       gpu_rsmi_get_api);
    apis_set(ops->init,          gpu_rsmi_init);
    apis_set(ops->dispose,       gpu_rsmi_dispose);
    apis_set(ops->get_devices,   gpu_rsmi_get_devices);
    apis_set(ops->count_devices, gpu_rsmi_count_devices);
    apis_pin(ops->read,          gpu_rsmi_read, ok_pool);
    apis_set(ops->read_raw,      gpu_rsmi_read_raw);
    debug("Loaded RSMI");
}

void gpu_rsmi_get_api(uint *api)
{
    *api = API_RSMI;
}

state_t gpu_rsmi_init(ctx_t *c)
{
    state_t s = EAR_SUCCESS;
    timestamp_t time;
    int i;

    if (init) {
        return s;
    }
    debug("Initialized");
    init = 1;
    // Allocation
    pool = calloc(devs_count, sizeof(gpu_t));
    // Initializing pool (pool at 0 is not incorrect)
    timestamp_getfast(&time);
    for (i = 0; i < devs_count; ++i) {
        pool[i].time    = time;
        pool[i].correct = 1;
    }
    if (!ok_pool) {
        return s;
    }
    // Initializing monitoring thread suscription.
    sus = suscription();
    sus->call_main  = gpu_rsmi_pool;
    sus->time_relax = 2000;
    sus->time_burst = 1000;
    // Initializing monitoring thread.
    if (state_fail(s = sus->suscribe(sus))) {
        debug("GPU monitor FAILS");
        gpu_rsmi_dispose(c);
    }
    return s;
}

state_t gpu_rsmi_dispose(ctx_t *c)
{
    if (init) {
        monitor_unregister(sus);
        free(pool);
        init = 0;
    }
    return EAR_SUCCESS;
}

state_t gpu_rsmi_get_devices(ctx_t *c, gpu_devs_t **devs_in, uint *devs_count_in)
{
    char serial[32];
    rsmi_status_t r;
    int i;

    *devs_in = calloc(devs_count, sizeof(gpu_devs_t));
    //
    for (i = 0; i < devs_count; ++i) {
        if ((r = rsmi.get_serial(i, serial, 32)) != RSMI_STATUS_SUCCESS) {
            return_msg(EAR_ERROR, "RSMI error");
        }
        debug("D%d: %s", i, serial);
        (*devs_in)[i].serial = (ullong) atoll(serial);
        (*devs_in)[i].index  = i;
    }
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }

    return EAR_SUCCESS;
}

state_t gpu_rsmi_count_devices(ctx_t *c, uint *devs_count_in)
{
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }
    return EAR_SUCCESS;
}

static int is_working(int i)
{
    rsmi_process_info_t processes[32];
    uint                processes_count = 32;
    uint                devices[32];
    uint                devices_count = 32;
    int                 p, d;

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
    ullong temp_mc = 0;
    uint gpu_util;
    uint mem_util;
    rsmi_status_t s;

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
    metric->correct  = 1;
    //
    unused(s);

    return 1;
}

state_t gpu_rsmi_pool(void *p)
{
    timestamp_t time;
    double time_diff;
    gpu_t metric;
    int working;
    int i;

    debug("Pooling");
    //
    working = 0;
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

    return EAR_SUCCESS;
}

state_t gpu_rsmi_read(ctx_t *c, gpu_t *data)
{
    // Updating pool
    gpu_rsmi_pool(NULL);
    while (pthread_mutex_trylock(&lock));
    memcpy(data, pool, devs_count * sizeof(gpu_t));
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t gpu_rsmi_read_raw(ctx_t *c, gpu_t *data)
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
