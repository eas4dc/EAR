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

#include <math.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <common/config.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/system/symplug.h>
#include <metrics/common/oneapi.h>
#include <metrics/gpu/archs/oneapi.h>

#define ONE_API_RELAX 5000
#define ONE_API_BURST 3000

#define VGPU_DATA 3

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static gpu_t         *pool; // change to sysmans_pool
static timestamp_t    pool_time1;
static timestamp_t    pool_time2;
static uint           is_initialized;
static uint           is_pooling;
static suscription_t *sus;
static uint           devs_count;
static ze_handlers_t *devs;
static ze_t           ze;
static int            mode;

#define if_fail(function, action)                                                                                      \
    if ((z = function) != ZE_RESULT_SUCCESS) {                                                                         \
        debug("Failed " #function ": %s (0x%x)", oneapi_strerror(z), z);                                               \
        action;                                                                                                        \
    }

static void load_atfork()
{
    // Just lock control is enough
    pthread_mutex_unlock(&lock);
}

void gpu_oneapi_load(gpu_ops_t *ops, int force_api)
{
    debug("Received API %d", force_api);
    // If api is API_EARD, then EARD provides the gpu_t reading.
    if (API_IS(force_api, API_DEFAULT)) {
        is_pooling = 1;
    } else if (API_IS(force_api, API_FREE)) {
        is_pooling = oneapi_is_privileged();
    }
    // Just sysman is needed
    if (state_fail(oneapi_open_sysman(&ze))) {
        debug("oneapi_open_sysman failed: %s", state_msg);
        return;
    }
    if (state_fail(oneapi_get_handlers(&devs, &devs_count))) {
        debug("oneapi_get_devices failed: %s", state_msg);
        return;
    }
    // Allocation
    pool = calloc(devs_count, sizeof(gpu_t));
    // Atfork control
    pthread_atfork(NULL, NULL, load_atfork);

    apis_set(ops->get_info   , gpu_oneapi_get_info);
    apis_set(ops->get_devices, gpu_oneapi_get_devices);
    apis_set(ops->init       , gpu_oneapi_init);
    apis_set(ops->dispose    , gpu_oneapi_dispose);
    apis_set(ops->set_monitoring_mode, gpu_oneapi_set_monitoring_mode);
    apis_pin(ops->read       , gpu_oneapi_read, is_pooling);
    apis_set(ops->read_raw   , gpu_oneapi_read_raw);
    apis_set(ops->data_diff  , gpu_oneapi_data_diff);
}

void gpu_oneapi_get_info(apinfo_t *info)
{
    info->api        = API_ONEAPI;
    info->devs_count = devs_count;
}

void gpu_oneapi_get_devices(gpu_devs_t **devs_in, uint *devs_count_in)
{
    int dv;
    if (devs_in != NULL) {
        *devs_in = calloc(devs_count, sizeof(gpu_devs_t));
        //
        for (dv = 0; dv < devs_count; ++dv) {
            (*devs_in)[dv].serial = devs[dv].uuid;
            (*devs_in)[dv].index  = dv;
        }
    }
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }
}

state_t gpu_oneapi_init(ctx_t *c)
{
    state_t s = EAR_SUCCESS;
    timestamp_t time;
    int i;

    if (is_initialized) {
        return s;
    }
    // Initializing pool (pool at 0 is not incorrect)
    timestamp_getfast(&time);
    for (i = 0; i < devs_count; ++i) {
        pool[i].time    = time;
        pool[i].correct = 1;
    }
    if (is_pooling) {
        // Monitor suscription
        sus             = suscription();
        sus->call_main  = gpu_oneapi_pool;
        sus->time_relax = ONE_API_RELAX;
        sus->time_burst = ONE_API_BURST;
        // Initializing monitoring thread.
        if (state_ok(s = sus->suscribe(sus))) {
            is_initialized = 1;
        }
    } // is_pooling
    return s;
}

state_t gpu_oneapi_dispose(ctx_t *c)
{
    if (is_initialized) {
        monitor_unregister(sus);
        is_initialized = 0;
    }
    return EAR_SUCCESS;
}

void gpu_oneapi_set_monitoring_mode(int mode_in)
{
    mode = mode_in;
    if (mode == MONITORING_MODE_IDLE && is_pooling) {
        debug("Setting RELAX mode in GPU pooling");
        monitor_relax(sus);
    }
    if (mode == MONITORING_MODE_RUN && is_pooling) {
        debug("Setting BURST mode in GPU pooling");
        monitor_burst(sus, 0);
    }
}

static state_t oneapi_read_power(int dv, gpu_t *data)
{
    zes_power_energy_counter_t info;
    ze_result_t z;
    double joules;
    uint d;
    // Retrieving the domains
    data->energy_j = 0.0;
    // It seems CARD matches DOMAIN0 (all the PCI CARD)
    if (devs[dv].scard_count > 0) {
        if (ze.sPowerGetEnergyCounter(devs[dv].scard, &info) == ZE_RESULT_SUCCESS) {
            debug("DEV%d CARDP DOMAIN%d: %020lu uJ at %lu uS",
                dv, 0, info.energy, info.timestamp);
            data->energy_j = ((double) info.energy) / 1000000.0;
            #if !SHOW_DEBUGS
            return EAR_SUCCESS;
            #endif
        }
    }
    // If card doesn't exists
    for (d = 0; d < devs[dv].spowers_count; ++d) {
        if_fail(ze.sPowerGetEnergyCounter(devs[dv].spowers[d], &info), continue);
        debug("DEV%d POWER DOMAIN%d: %020lu uJ at %lu uS (sub %d)",
            dv, d, info.energy, info.timestamp, devs[dv].spowers_props[d].subdeviceId);
        // SUBDEVICE -1 seems to be the whole GPU, but if not, we are
        // taking the greatest joules value
        joules = ((double) info.energy) / 1000000.0;
        if (joules > data->energy_j) {
            data->energy_j = joules;
        }
        #if !SHOW_DEBUGS
        if (devs[dv].spowers_props[d].subdeviceId == -1) {
            return EAR_SUCCESS;
        }
        #endif
    }
    return EAR_SUCCESS;
}

static state_t oneapi_read_frequency(int dv, gpu_t *data)
{
    zes_freq_state_t info;
    uint gpu_count = 0;
    uint mem_count = 0;
    ze_result_t z;
    int d;
    // Retrieving the domains
    for (d = devs[dv].sfreqs_count - 1; d >= 0; --d) {
        if_fail(ze.sFrequencyGetState(devs[dv].sfreqs[d], &info), continue);
        debug("DEV%d FREQ DOMAIN%d: act/eff/tdp %.0lf/%.0lf/%.0lf (%.0lf to %.0lf MHz) (sub %d)",
            dv, d, info.actual, info.efficient, info.tdp, devs[dv].sfreqs_props[d].max,
            devs[dv].sfreqs_props[d].min, devs[dv].sfreqs_props[d].subdeviceId);
        // Taking DOMAIN0
        if (devs[dv].sfreqs_props[d].type == ZES_FREQ_DOMAIN_GPU) {
            data->freq_gpu += (ulong) info.actual; // MHz
            gpu_count += 1;
        }
        if (devs[dv].sfreqs_props[d].type == ZES_FREQ_DOMAIN_MEMORY) {
            data->freq_mem += (ulong) info.actual; // MHz
            mem_count += 1;
        }
    }
    if (gpu_count > 1)
        data->freq_gpu /= gpu_count;
    if (mem_count > 1)
        data->freq_mem /= mem_count;
    data->freq_gpu = data->freq_gpu * 1000LU; // KHZ
    data->freq_mem = data->freq_mem * 1000LU; // KHZ
    return EAR_SUCCESS;
}

static const char *engtostr(int eng, int *prio)
{
    #define engstr(e, p)                                                                                                   \
    if (eng == ZES_ENGINE_GROUP_##e) {                                                                                 \
        if (prio != NULL) {                                                                                            \
            *prio = p;                                                                                                 \
        }                                                                                                              \
        return #e;                                                                                                     \
    }
    #define NO_PRIO 10

    if (prio != NULL) {
        *prio = NO_PRIO;
    }
    engstr(ALL,                      1);
    engstr(COMPUTE_ALL,              2);
    engstr(COMPUTE_SINGLE,           3);
    engstr(RENDER_ALL,               4);
    engstr(RENDER_SINGLE,            5);
    engstr(3D_RENDER_COMPUTE_ALL,    6);
    engstr(3D_ALL,                   7);
    engstr(3D_SINGLE,                8);
    engstr(MEDIA_ALL,                9);
    engstr(MEDIA_DECODE_SINGLE,      NO_PRIO);
    engstr(MEDIA_ENCODE_SINGLE,      NO_PRIO);
    engstr(MEDIA_ENHANCEMENT_SINGLE, NO_PRIO);
    engstr(COPY_ALL,                 NO_PRIO);
    engstr(COPY_SINGLE,              NO_PRIO);
    return "UNKNOWN";
}

static state_t oneapi_read_utilization(int dv, gpu_t *data)
{
    static zes_engine_stats_t save[32][64]; // 32 devices, 64 domains
    zes_engine_stats_t info;
    int                prio_count = 0;
    int                prio_set = NO_PRIO;
    int                prio;
    ze_result_t        z;
    uint               d;

    for (d = 0; d < devs[dv].sengines_count; ++d) {
        if_fail(ze.sEngineGetActivity(devs[dv].sengines[d], &info), continue);
        #if SHOW_DEBUGS
        debug("DEV%d ENGINE DOMAIN%d: %lu active uS (%lu timestamp) (sub %d) (%s)",
            dv, d, info.activeTime, info.timestamp, devs[dv].sengines_props[d].subdeviceId,
            engtostr(devs[dv].sengines_props[d].type, NULL));
        #endif
        // Getting the priority
        engtostr(devs[dv].sengines_props[d].type, &prio);
        // Is new or already found?
        if (prio != NO_PRIO) {
            ulong adiff = info.activeTime - save[dv][d].activeTime;
            ulong tdiff = info.timestamp  - save[dv][d].timestamp;
            if (prio < prio_set) {
                debug("DEV%d ENGINE DOMAIN%d: selecting %s", dv, d,
                    engtostr(devs[dv].sengines_props[d].type, NULL));
                data->util_gpu = (tdiff > 0LU) ? adiff * 100 / tdiff : 0LU;
                prio_set       = prio;
                prio_count     = 1;
            } else if (prio == prio_set) {
                data->util_gpu += (tdiff > 0LU) ? adiff * 100 / tdiff : 0LU;
                prio_count += 1;
            }
            memcpy(&save[dv][d], &info, sizeof(zes_engine_stats_t));
        }
    }
    if (prio_count > 1) {
        data->util_gpu /= prio_count;
    }
    if (data->util_gpu > 0) {
        data->working = 1;
    }
    data->util_gpu = ear_min(data->util_gpu, 100);
    return EAR_SUCCESS;
}

static state_t oneapi_read_temperature(int dv, gpu_t *data)
{
    zes_psu_state_t state;
    uint gpu_count = 0;
    uint mem_count = 0;
    ze_result_t z;
    double temp;
    uint s;

    for (s = 0; s < devs[dv].stemps_count; ++s) {
        if_fail(ze.sTemperatureGetState(devs[dv].stemps[s], &temp), continue);
        debug("DEV%d TEMP DOMAIN%d: %lf celsius (%lf max) (type %d)", dv, s, temp,
            devs[dv].stemps_props[s].maxTemperature, devs[dv].stemps_props[s].type);
        if (devs[dv].stemps_props[s].type != ZES_TEMP_SENSORS_GPU) {
            data->temp_gpu += (ulong) temp;
            gpu_count += 1;
        }
        if (devs[dv].stemps_props[s].type != ZES_TEMP_SENSORS_MEMORY) {
            data->temp_mem += (ulong) temp;
            mem_count += 1;
        }
    }
    if (gpu_count > 1) data->temp_gpu /= gpu_count;
    if (mem_count > 1) data->temp_mem /= mem_count;
    for (s = 0; s < devs[dv].spsus_count; ++s) {
        if_fail(ze.sPsuGetState(devs[dv].spsus[s], &state), continue);
        debug("DEV%d PSU DOMAIN%d: %d celsius", dv, s, state.temperature);
    }
    return EAR_SUCCESS;
}

static void verbose_single(int vl, int dv, gpu_t *gpu)
{
    verbose(vl, "GPU[%d] energy    %lf", dv, gpu->energy_j);
    verbose(vl, "GPU[%d] power     %lf", dv, gpu->power_w);
    verbose(vl, "GPU[%d] util      %lu", dv, gpu->util_gpu);
    verbose(vl, "GPU[%d] mem util  NA", dv);
    verbose(vl, "GPU[%d] temp_gpu  %lu", dv, gpu->temp_gpu);
    verbose(vl, "GPU[%d] temp_mgpu %lu", dv, gpu->temp_mem);
    verbose(vl, "GPU[%d] freq_gpu  %lu", dv, gpu->freq_gpu);
    verbose(vl, "GPU[%d] freq_mem  %lu", dv, gpu->freq_mem);
}

static void read_single(int dv, gpu_t *gpu)
{
    // Cleanin
    memset(gpu, 0, sizeof(gpu_t));
    // Reading
    oneapi_read_power(dv, gpu);
    // If monitoring mode is RUN, it means there are jobs running in GPU and we
    // can read more metrics without the fear of power consumption being increased.
    if (mode == MONITORING_MODE_RUN) {
        oneapi_read_frequency(dv, gpu);
        oneapi_read_utilization(dv, gpu);
        oneapi_read_temperature(dv, gpu);
    }
    verbose_single(VGPU_DATA, dv, gpu);
}

state_t gpu_oneapi_pool(void *p)
{
    static double power_last[MAX_GPUS_SUPPORTED] = { 0 }; // Last valid power
    timestamp_t time;
    double time_diff;
    gpu_t current;
    double power;
    int add;
    int dv;

    // Locking
    while (pthread_mutex_trylock(&lock));
    // ¿Add or reset?
    add = (mode == MONITORING_MODE_RUN);
    // Time operations
    timestamp_getfast(&time);
    time_diff = (double) timestamp_diff(&time, &pool[0].time, TIME_USECS);
    #if 1
    // If time is not enough we just return. Byt why?
    if (time_diff < ((ONE_API_BURST * 1000) / 2)) {
        pthread_mutex_unlock(&lock);
        return EAR_SUCCESS;
    }
    #endif
    time_diff = time_diff / 1000000.0;
    // Adding to the pool
    for (dv = 0; dv < devs_count; ++dv) {
        read_single(dv, &current);
        pool[dv].time      = time;
        pool[dv].samples  += 1;
        pool[dv].freq_mem  = (add) ? pool[dv].freq_mem + current.freq_mem: 0;
        pool[dv].freq_gpu  = (add) ? pool[dv].freq_gpu + current.freq_gpu: 0;
        pool[dv].util_mem  = (add) ? pool[dv].util_mem + 1: 0; // 2nd sample counter
        pool[dv].util_gpu  = (add) ? pool[dv].util_gpu + current.util_gpu: 0;
        pool[dv].temp_gpu  = (add) ? pool[dv].temp_gpu + current.temp_gpu: 0;
        pool[dv].temp_mem  = (add) ? pool[dv].temp_gpu + current.temp_mem: 0;
        // Power processing
        power = ((current.energy_j - pool[dv].energy_j) / time_diff);
        // To add extra robustness
        if (pool[dv].samples > 1) {
            if (!isnan(power) && !isinf(power)) {
                // Valid power
                pool[dv].power_w += power;
                power_last[dv]    = power;
            } else {
                pool[dv].power_w += power_last[dv];
            }
        } else {
            pool[dv].power_w = 0.0;
        }
        pool[dv].energy_j  = current.energy_j;
        pool[dv].working   = current.working;
        pool[dv].correct   = 1;
        debug("dev | metric   | value    ");
        debug("--- | -------- | -----    ");
        debug("%d   | time     | %0.2lf s", dv, time_diff);
        debug("%d   | samps    | %lu     ", dv, pool[dv].samples);
        debug("%d   | freq_gpu | %lu MHz ", dv, pool[dv].freq_gpu);
        debug("%d   | freq_mem | %lu MHz ", dv, pool[dv].freq_mem);
        debug("%d   | util_gpu | %lu %%  ", dv, pool[dv].util_gpu);
        debug("%d   | util_mem | %lu %%  ", dv, pool[dv].util_mem);
        debug("%d   | temp_gpu | %lu cels", dv, pool[dv].temp_gpu);
        debug("%d   | temp_mem | %lu cels", dv, pool[dv].temp_mem);
        debug("%d   | energy   | %0.2lf J", dv, pool[dv].energy_j);
        debug("%d   | power    | %0.2lf W", dv, pool[dv].power_w);
        debug("%d   | working  | %d      ", dv, pool[dv].working);
    }
    // Copying time
    pool_time1 = pool_time2;
    // Unlock
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t gpu_oneapi_read(ctx_t *c, gpu_t *data)
{
    gpu_oneapi_pool(NULL);
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

static void static_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff, int i)
{
    ulong d3_samples2 = (data_diff[i].util_mem > 0)? data_diff[i].util_mem: 1;
    gpu_t *d3 = &data_diff[i];
    gpu_t *d2 = &data2[i];
    gpu_t *d1 = &data1[i];

    if (d2->correct != 1 || d1->correct != 1) {
        return;
    }
    // Averages
    if ((d3->samples = d2->samples - d1->samples) == 0) {
        memset(d3, 0, sizeof(gpu_t));
        return;
    }
    // Computing time
    double time_f = timestamp_diff(&d2->time, &d1->time, TIME_SECS);
    debug("static_data_diff accumulated time diff %lf (d2 %lu d1 %lu)",
          time_f, d2->time.tv_sec, d1->time.tv_sec);
    // No overflow control is required (64-bits are enough)
    d3->freq_gpu = (d2->freq_gpu - d1->freq_gpu) / d3_samples2;
    d3->freq_mem = (d2->freq_mem - d1->freq_mem) / d3_samples2;
    d3->util_gpu = (d2->util_gpu - d1->util_gpu) / d3_samples2;
    d3->util_mem = 0;
    d3->temp_gpu = (d2->temp_gpu - d1->temp_gpu) / d3_samples2;
    d3->temp_mem = (d2->temp_mem - d1->temp_mem) / d3_samples2;
    d3->power_w  = (d2->power_w  - d1->power_w)  / d3->samples;
    d3->energy_j = (d2->energy_j - d1->energy_j);
    d3->working  = (d2->working);
    d3->correct  = 1;

    if (d3->power_w == 0) {
        d3->power_w = d3->energy_j / time_f;
        debug("WARNING: Computing power using energy : %lf power = %lf energy / %lf secs",
              d3->power_w, d3->energy_j, time_f);
    }
    #if 1
    debug("%d freq gpu (KHz), %lu = %lu - %lu", i, d3->freq_gpu, d2->freq_gpu, d1->freq_gpu);
    debug("%d freq mem (KHz), %lu = %lu - %lu", i, d3->freq_mem, d2->freq_mem, d1->freq_mem);
    debug("%d power    (W)  , %0.2lf = %0.2lf - %0.2lf", i, d3->power_w, d2->power_w, d1->power_w);
    debug("%d energy   (J)  , %0.2lf = %0.2lf - %0.2lf", i, d3->energy_j, d2->energy_j, d1->energy_j);
    #endif
}

void gpu_oneapi_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
    int i;
    for (i = 0; i < devs_count; i++) {
        static_data_diff(data2, data1, data_diff, i);
    }
}