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

#include <errno.h>
#include <fcntl.h>

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
/* Max only when the driver doesn't report power1_rated_max */
#define DUMMY_TDP 600

#ifndef PVC_HWMON_ROOT
#define PVC_HWMON_ROOT "/sys/class/hwmon"
#endif

/* Per device: hwmon index, rated (default) power limit and PL1 at init, W (0 = unknown) */
static uint *pvc_hwmon_ids;
static uint32_t *pvc_rated_w;

/* Reads a microwatt hwmon file of device dv: W, or 0 if it can't be read */
static uint32_t pvc_read_uw_file(uint dv, const char *file)
{
    char path[256], buffer[32];
    ssize_t n;
    int fd;
    snprintf(path, sizeof(path), "%s/hwmon%u/%s", PVC_HWMON_ROOT, pvc_hwmon_ids[dv], file);
    if ((fd = open(path, O_RDONLY)) < 0)
        return 0;
    n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (n <= 0)
        return 0;
    buffer[n] = '\0';
    return (uint32_t) (strtoull(buffer, NULL, 10) / 1000000ULL);
}

static uint32_t pvc_max_w(uint dv)
{
    return pvc_rated_w[dv] ? pvc_rated_w[dv] : DUMMY_TDP;
}

static void init_pv_freqs()
{
    gpu_clock_list = (ulong **) calloc(pvc_gpus_num_devices, sizeof(ulong *));
    if (gpu_clock_list == NULL) {
        verbose(0, "Error not enough memory");
        exit(0);
    }
    for (uint dv = 0; dv < pvc_gpus_num_devices; dv++) {
        gpu_clock_list[dv] = calloc(1, sizeof(ulong));
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
    free(pvc_rated_w);
    free(pvc_hwmon_ids);
    pvc_rated_w   = NULL;
    pvc_hwmon_ids = NULL;
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
    /* ids are kept: the power files are read per device */
    pvc_hwmon_ids = ids;
    if ((pvc_rated_w = calloc(id_count, sizeof(uint32_t))) == NULL) {
        pthread_mutex_unlock(&lock);
        return_msg(s, strerror(errno));
    }
    for (i = 0; i < h->id_count; ++i) {
        pvc_rated_w[i] = pvc_read_uw_file(i, "power1_rated_max");
        debug("PVC%d: rated power %u W", i, pvc_rated_w[i]);
    }
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
    if (!pvc_hwmon_meter_initialized) {
        return EAR_ERROR;
    }
    if (devs_in != NULL) {
        *devs_in = calloc(pvc_gpus_num_devices, sizeof(gpu_devs_t));
        for (dv = 0; dv < pvc_gpus_num_devices; ++dv) {
            (*devs_in)[dv].serial = 0;
            (*devs_in)[dv].index  = dv;
        }
    }
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

/* Power caps: one uint32_t per device, as mgt_gpu_ops_t declares. The range comes from
 * the i915 hwmon files: power1_rated_max (default limit) is the max, power1_max the PL1
 * in force; the driver has no minimum (0, unknown). */
state_t mgt_pvc_hwmon_power_cap_get_current(ctx_t *c, uint32_t *watts)
{
    if (watts != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++) {
            uint32_t pl1 = pvc_read_uw_file(dv, "power1_max");
            watts[dv]    = pl1 ? pl1 : pvc_max_w(dv);
        }
    }
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_power_cap_get_default(ctx_t *c, uint32_t *watts)
{
    if (watts != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            watts[dv] = pvc_max_w(dv);
    }
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_power_cap_get_rank(ctx_t *c, uint32_t *watts_min, uint32_t *watts_max)
{
    if (watts_max != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            watts_max[dv] = pvc_max_w(dv);
    }
    if (watts_min != NULL) {
        for (uint dv = 0; dv < pvc_gpus_num_devices; dv++)
            watts_min[dv] = 0;
    }
    return EAR_SUCCESS;
}

/* Not implemented: caps are not written to the device */
state_t mgt_pvc_hwmon_power_cap_reset(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_pvc_hwmon_power_cap_set(ctx_t *c, uint32_t *watts)
{
    debug("mgt_pvc_hwmon_power_cap_set");
    return EAR_SUCCESS;
}
