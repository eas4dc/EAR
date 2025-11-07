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
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/output/debug.h>
#include <common/sizes.h>
#include <common/system/symplug.h>
#include <management/gpu/archs/mgt_pvc_power_hwmon.h>
#include <metrics/common/apis.h>
#include <metrics/common/hwmon.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static state_t mgt_pvc_hwmon_static_init(ctx_t *c);

static uint pvc_hwmon_meter_initialized = 0;
static uint pvc_gpus_num_devices        = 0;
static uint gpu_apis_loaded             = 0;
static ulong **gpu_clock_list;
static uint *gpu_clock_lens;

typedef struct hwfds_s {
    uint id_count;
    uint *fd_count;
    int **fds;
} hwfds_t;

static hwfds_t local_context;
static bool pvc_gpus_found = false;

#define DUMMY_FREQ 1600000
#define DUMMY_TDP  600

static void init_pv_freqs()
{
    gpu_clock_list = (ulong **) calloc(pvc_gpus_num_devices, sizeof(ulong *));
    if (gpu_clock_list == NULL) {
        verbose(0, "Error not enough memory");
        exit(0);
    }
    for (uint dv = 0; dv < pvc_gpus_num_devices; dv++) {
        gpu_clock_list[dv] = calloc(1, sizeof(uint));
        if (gpu_clock_list[dv] == NULL) {
            verbose(0, "Error not enough memory");
            exit(0);
        }
        gpu_clock_list[dv][0] = DUMMY_FREQ;
    }

    gpu_clock_lens = calloc(pvc_gpus_num_devices, sizeof(uint));
    if (gpu_clock_lens == NULL) {
        verbose(0, "Error not enough memory");
        exit(0);
    }
    for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
        gpu_clock_lens[dv] = 1;
}

static void release_static_data()
{
    free(gpu_clock_lens);
    for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
        free(gpu_clock_list[dv]);
    free(gpu_clock_list);
}

state_t mgt_gpu_pvc_hwmon_load(mgt_gpu_ops_t *ops)
{
    state_t s;
    debug("pvc_hwmon load asking for status");

    while (pthread_mutex_trylock(&lock))
        ;
    if (pvc_gpus_num_devices == 0) {
        if (state_ok(s = hwmon_find_drivers("i915", NULL, NULL))) {
            pvc_gpus_found = true;
        }
    }
    pthread_mutex_unlock(&lock);
    debug("PVC GPUS found");
    if (pvc_gpus_found) {
        apis_set(ops->init, mgt_pvc_hwmon_init);
        apis_set(ops->dispose, mgt_pvc_hwmon_dispose);
        // Queries
        apis_set(ops->get_api, mgt_pvc_hwmon_get_api);
        apis_set(ops->get_devices, mgt_pvc_hwmon_get_devices);
        apis_set(ops->count_devices, mgt_pvc_hwmon_count_devices);

        // DUMMY
        apis_put(ops->freq_limit_get_current, mgt_pvc_hwmon_freq_limit_get_current);
        apis_put(ops->freq_limit_get_default, mgt_pvc_hwmon_freq_limit_get_default);
        apis_put(ops->freq_limit_get_max, mgt_pvc_hwmon_freq_limit_get_max);
        apis_put(ops->power_cap_get_current, mgt_pvc_hwmon_power_cap_get_current);
        apis_put(ops->power_cap_get_default, mgt_pvc_hwmon_power_cap_get_default);
        apis_put(ops->power_cap_get_rank, mgt_pvc_hwmon_power_cap_get_rank);
        // Commands (put if empty)
        apis_put(ops->freq_limit_reset, mgt_pvc_hwmon_freq_limit_reset);
        apis_put(ops->freq_limit_set, mgt_pvc_hwmon_freq_limit_set);
        apis_put(ops->freq_list, mgt_pvc_hwmon_freq_get_available);
        apis_put(ops->power_cap_reset, mgt_pvc_hwmon_power_cap_reset);
        apis_put(ops->power_cap_set, mgt_pvc_hwmon_power_cap_set);
    }

    return (pvc_gpus_found ? mgt_pvc_hwmon_static_init(no_ctx) : EAR_ERROR);
}

void mgt_pvc_hwmon_get_api(uint *api)
{
    *api = API_PVC_HWMON;
}

state_t mgt_pvc_hwmon_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

static state_t mgt_pvc_hwmon_static_init(ctx_t *c)
{
    uint id_count;
    uint *ids;
    state_t s;
    int i;

    /* MUTEX pending */

    debug("mgt_pvc_hwmon_init asking for init");

    while (pthread_mutex_trylock(&lock))
        ;
    if (pvc_hwmon_meter_initialized) {
        pthread_mutex_unlock(&lock);
        return EAR_SUCCESS;
    }

    // Looking for ids
    debug("pvc_hwmon_init looking for devices");
    if (xtate_fail(s, hwmon_find_drivers("i915", &ids, &id_count))) {
        pthread_mutex_unlock(&lock);
        return s;
    }
    if (id_count == 0) {
        pthread_mutex_unlock(&lock);
        return EAR_ERROR;
    }
    debug("PVC %d devices found", id_count);
    pvc_gpus_num_devices = id_count;
    // Allocating file descriptors)
    hwfds_t *h = &local_context;
    /* fd is a per device a per num gpu vector */
    if ((h->fds = calloc(id_count, sizeof(int *))) == NULL) {
        pthread_mutex_unlock(&lock);
        return_msg(s, strerror(errno));
    }
    if ((h->fd_count = calloc(id_count, sizeof(uint))) == NULL) {
        pthread_mutex_unlock(&lock);
        return_msg(s, strerror(errno));
    }
    h->id_count = id_count;
    //
    for (i = 0; i < h->id_count; ++i) {
        if (xtate_fail(s, hwmon_open_files(ids[i], Hwmon_pvc_energy.energy_j, &h->fds[i], &h->fd_count[i], 1))) {
            pthread_mutex_unlock(&lock);
            return s;
        }
    }
    // Freeing ids space
    free(ids);
    debug("init succeded");
    gpu_apis_loaded++;

    pvc_hwmon_meter_initialized = 1;
    init_pv_freqs();

    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_get_devices(ctx_t *c, gpu_devs_t **devs_in, uint *devs_count_in)
{
    int dv;

    debug("mgt_pvc_hwmon_get_devices");
    if (!pvc_hwmon_meter_initialized)
        return EAR_ERROR;
    debug("mgt_pvc_hwmon_get_devices allocating data for %u GPUS", pvc_gpus_num_devices);
    if (devs_in != NULL) {
        *devs_in = calloc(pvc_gpus_num_devices, sizeof(gpu_devs_t));
        //
        for (dv = 0; dv < pvc_gpus_num_devices; ++dv) {
            (*devs_in)[dv].serial = 0;
            (*devs_in)[dv].index  = dv;
        }
    }
    debug("Serial info allocated");
    if (devs_count_in != NULL) {
        *devs_count_in = pvc_gpus_num_devices;
    }
    return EAR_SUCCESS;
}

static state_t get_context(ctx_t *c, hwfds_t **h)
{
    *h = &local_context;
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_dispose(ctx_t *c)
{
    int i;
    hwfds_t *h;
    state_t s;

    debug("mgt_pvc_hwmon_dispose");
    if (!pvc_hwmon_meter_initialized)
        return EAR_ERROR;

    while (pthread_mutex_trylock(&lock))
        ;

    gpu_apis_loaded--;
    if (gpu_apis_loaded == 0) {
        debug("warning, GPU is going to be release!! ");
        gpu_apis_loaded = 1;
        pthread_mutex_unlock(&lock);
        return EAR_SUCCESS;
    }

    /* This should be the normal behavior */
    if (gpu_apis_loaded) {
        pthread_mutex_unlock(&lock);
        return EAR_SUCCESS;
    }
    if (xtate_fail(s, get_context(c, &h))) {
        pthread_mutex_unlock(&lock);
        return s;
    }
    for (i = 0; i < h->id_count; ++i) {
        hwmon_close_files(h->fds[i], h->fd_count[i]);
    }
    free(h->fd_count);
    free(h->fds);
    pvc_hwmon_meter_initialized = 0;

    release_static_data();
    pthread_mutex_unlock(&lock);

    return EAR_SUCCESS;
}

// Data
state_t mgt_pvc_hwmon_count_devices(ctx_t *c, uint *count)
{
    hwfds_t *h;
    state_t s;

    debug("mgt_pvc_hwmon_count_devices");
    if (!pvc_hwmon_meter_initialized)
        return EAR_ERROR;
    if (xtate_fail(s, get_context(c, &h))) {
        return s;
    }
    if ((count != NULL) && (h != NULL)) {
        //*count = h->id_count;
        // To be compatible with other APIs we will use
        // gpu granularity for now
        *count = pvc_gpus_num_devices;
    }

    return EAR_SUCCESS;
}

/****** DUMMY functions */
state_t mgt_pvc_hwmon_freq_limit_get_current(ctx_t *c, ulong *khz)
{
    if (khz != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            khz[dv] = DUMMY_FREQ;
    }
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_freq_limit_get_default(ctx_t *c, ulong *khz)
{
    if (khz != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            khz[dv] = DUMMY_FREQ;
    }
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_freq_limit_get_max(ctx_t *c, ulong *khz)
{
    if (khz != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            khz[dv] = DUMMY_FREQ;
    }
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_freq_limit_reset(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_freq_limit_set(ctx_t *c, ulong *khz)
{
    debug("mgt_pvc_hwmon_freq_limit_set");
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_freq_get_available(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
    if (list_khz != NULL) {
        *list_khz = (const ulong **) gpu_clock_list;
    }
    if (list_len != NULL) {
        *list_len = (const uint *) gpu_clock_lens;
    }
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_power_cap_get_current(ctx_t *c, ulong *watts)
{
    if (watts != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            watts[dv] = DUMMY_TDP;
    }
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_power_cap_get_default(ctx_t *c, ulong *watts)
{
    if (watts != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            watts[dv] = DUMMY_TDP;
    }
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
    if (watts_max != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            watts_max[dv] = DUMMY_TDP;
    }
    if (watts_min != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            watts_min[dv] = DUMMY_TDP;
    }
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_power_cap_reset(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_power_cap_set(ctx_t *c, ulong *watts)
{
    debug("mgt_pvc_hwmon_power_cap_set");
    return EAR_SUCCESS;
}
