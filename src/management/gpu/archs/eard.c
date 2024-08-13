/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1
//#define FAKE_GPUS   1

#include <stdlib.h>
#include <common/utils/serial_buffer.h>
#include <common/config/config_install.h>
#include <management/gpu/archs/eard.h>
#include <daemon/local_api/eard_api_rpc.h>

static int         eard;
// Received by EAR
static uint        root_devs_count;
static ulong      *root_freq_default;
static ulong      *root_freq_max;
static ulong     **root_freqs_available;
static uint       *root_freqs_count;
static ulong      *root_power_default;
static ulong      *root_power_max;
static ulong      *root_power_min;
static gpu_devs_t *root_devs;
// Formatted by other APIs
static uint        user_devs_count;
static ulong      *user_freq_default;
static ulong      *user_freq_max;
static ulong     **user_freqs_available;
static uint       *user_freqs_count;
static ulong      *user_power_default;
static ulong      *user_power_max;
static ulong      *user_power_min;
static gpu_devs_t *user_devs;

static state_t recv_static_data(mgt_gpu_ops_t *ops)
{
    wide_buffer_t b;
    uint eard_api;
    state_t s;
    int i;

    if (state_fail(s = eard_rpc(RPC_MGT_GPU_GET_API, NULL, 0, (char *) &eard_api, sizeof(int)))) {
        return s;
    }
    debug("Received API %u", eard_api);
    if (eard_api == API_NONE || eard_api == API_DUMMY) {
        debug("EARD (daemon) has loaded DUMMY/NONE API");
        return EAR_ERROR;
    }
    if (state_fail(s = eard_rpc_buffered(RPC_MGT_GPU_GET_DEVICES, NULL, 0, (char **) &b, NULL))) {
        return s;
    }
    serial_copy_elem(&b, (char *) &root_devs_count, NULL);
    debug("Received from EARD %u GPUs", root_devs_count);
    if (root_devs_count == 0) {
        debug("There are no GPUs");
        return EAR_ERROR;
    }
    root_devs = (gpu_devs_t *) serial_copy_elem(&b, NULL, NULL); 
    user_devs_count = root_devs_count;
    // If there is an already loaded API
    if (ops->get_devices != NULL) {
        ops->get_devices(NULL, &user_devs, &user_devs_count);
    } else {
        // If not, just copy root devices
        user_devs = calloc(user_devs_count, sizeof(gpu_devs_t));
        memcpy(user_devs, root_devs, user_devs_count*sizeof(gpu_devs_t));
    }
    #if FAKE_GPUS
    user_devs_count = 1;
    #endif
    #ifdef SHOW_DEBUGS
    debug("Global GPUs:");
    for (i = 0; i < root_devs_count; ++i) {
        debug("GPU%d: %u_%llu", i, root_devs[i].index, root_devs[i].serial);
    }
    debug("Reservation GPUs:");
    for (i = 0; i < user_devs_count; ++i) {
        debug("GPU%d: %u_%llu", i, user_devs[i].index, user_devs[i].serial);
    }
    #endif

    /*
     * Second part
     */
    uint cn = root_devs_count;
    uint j, k;

    root_freq_default    = calloc(root_devs_count, sizeof(ulong));
    root_freq_max        = calloc(root_devs_count, sizeof(ulong));
    root_freqs_available = calloc(root_devs_count, sizeof(ulong *));
    root_freqs_count     = calloc(root_devs_count, sizeof(uint));
    root_power_default   = calloc(root_devs_count, sizeof(ulong));
    root_power_max       = calloc(root_devs_count, sizeof(ulong));
    root_power_min       = calloc(root_devs_count, sizeof(ulong));
    user_freq_default    = calloc(user_devs_count, sizeof(ulong));
    user_freq_max        = calloc(user_devs_count, sizeof(ulong));
    user_freqs_available = calloc(user_devs_count, sizeof(ulong *));
    user_freqs_count     = calloc(user_devs_count, sizeof(uint));
    user_power_default   = calloc(user_devs_count, sizeof(ulong));
    user_power_max       = calloc(user_devs_count, sizeof(ulong));
    user_power_min       = calloc(user_devs_count, sizeof(ulong));

    if (state_fail(s = eard_rpc(RPC_MGT_GPU_GET_FREQ_LIMIT_DEFAULT, NULL, 0, (char *) root_freq_default, cn*sizeof(ulong)))) {
        return s;
    }
    if (state_fail(s = eard_rpc(RPC_MGT_GPU_GET_FREQ_LIMIT_MAX, NULL, 0, (char *) root_freq_max, cn*sizeof(ulong)))) {
        return s;
    }
    // Freqs processing
    if (state_fail(s = eard_rpc_buffered(RPC_MGT_GPU_GET_FREQS_AVAILABLE, NULL, 0, (char **) &b, NULL))) {
        debug("Error during RPC: %s", state_msg);
        return s;
    }
    // Copying sizes
    serial_copy_elem(&b, (char *) root_freqs_count, NULL);
    // Copying frequencies
    for (i = 0; i < root_devs_count; ++i) {
        root_freqs_available[i] = (ulong *) serial_copy_elem(&b, NULL, NULL);
    }
    // Power
    if (state_fail(s = eard_rpc(RPC_MGT_GPU_GET_POWER_CAP_DEFAULT, NULL, 0, (char *) root_power_default, cn*sizeof(ulong)))) {
        return s;
    }
    // Max min processing
    if (state_fail(s = eard_rpc_buffered(RPC_MGT_GPU_GET_POWER_CAP_MAX, NULL, 0, (char **) &b, NULL))) {
        return s;
    }
    serial_copy_elem(&b, (char *) root_power_max, NULL);
    serial_copy_elem(&b, (char *) root_power_min, NULL);
    // Static shuffle
    for (i = 0; i < root_devs_count; ++i) {
    for (j = 0; j < user_devs_count; ++j) {
        if (root_devs[i].serial == user_devs[j].serial) {
            user_freqs_count[j]     = root_freqs_count[i];
            user_freqs_available[j] = calloc(root_freqs_count[i], sizeof(ulong));
            user_freq_default[j]    = root_freq_default[i];
            user_freq_max[j]        = root_freq_max[i];
            user_power_default[j]   = root_power_default[i];
            user_power_max[j]       = root_power_max[i];
            user_power_min[j]       = root_power_min[i];

            debug("D%d (root %d_%llu): #%u freqs, %lu KHz default, %lu KHz max, %lu W default, %lu-%lu W max-min",
                j, user_devs[j].index, user_devs[j].serial,
                user_freqs_count[j], user_freq_default[j], user_freq_max[j],
                user_power_default[j], user_power_max[j], user_power_min[j]);

            // Copying all the frequencies
            for (k = 0; k < root_freqs_count[i]; ++k) {
                user_freqs_available[j][k] = root_freqs_available[i][k];
                debug("--> %lu KHz", user_freqs_available[j][k]);
            }
        }
    }}

    return EAR_SUCCESS;
}

void mgt_gpu_eard_load(mgt_gpu_ops_t *ops, int _eard)
{
    eard = _eard;
    if (!eard) {
        debug("EARD (daemon) not required");
        return;
    }
    if (!eards_connected()) {
        debug("EARD (daemon) not connected");
        return;
    }
    if (state_fail(recv_static_data(ops))) {
        return;
    }
    apis_put(ops->init                  , mgt_gpu_eard_init);
    apis_put(ops->dispose               , mgt_gpu_eard_dispose);
    apis_put(ops->get_api               , mgt_gpu_eard_get_api);
    apis_put(ops->get_devices           , mgt_gpu_eard_get_devices);
    apis_put(ops->count_devices         , mgt_gpu_eard_count_devices);
    apis_put(ops->freq_limit_get_current, mgt_gpu_eard_freq_limit_get_current);
    apis_put(ops->freq_limit_get_default, mgt_gpu_eard_freq_limit_get_default);
    apis_put(ops->freq_limit_get_max    , mgt_gpu_eard_freq_limit_get_max);
    apis_put(ops->power_cap_get_current , mgt_gpu_eard_power_cap_get_current);
    apis_put(ops->power_cap_get_default , mgt_gpu_eard_power_cap_get_default);
    apis_put(ops->power_cap_get_rank    , mgt_gpu_eard_power_cap_get_rank);
    apis_put(ops->freq_limit_reset      , mgt_gpu_eard_freq_limit_reset);
    apis_put(ops->freq_limit_set        , mgt_gpu_eard_freq_limit_set);
    apis_put(ops->freq_list             , mgt_gpu_eard_freq_list);
    apis_put(ops->power_cap_reset       , mgt_gpu_eard_power_cap_reset);
    apis_put(ops->power_cap_set         , mgt_gpu_eard_power_cap_set);
    debug("Loaded EARD");
}

void mgt_gpu_eard_get_api(uint *api)
{
    *api = API_EARD;
}

state_t mgt_gpu_eard_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_eard_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_eard_get_devices(ctx_t *c, gpu_devs_t **devs, uint *devs_count)
{
    *devs = calloc(user_devs_count, sizeof(gpu_devs_t));
    memcpy(*devs, user_devs, user_devs_count*sizeof(gpu_devs_t));
    if (devs_count != NULL) {
        *devs_count = user_devs_count;
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_eard_count_devices(ctx_t *c, uint *dev_count)
{
    *dev_count = user_devs_count;
    return EAR_SUCCESS;
}

static state_t shuffle_in(ulong *user_list, ulong *root_list, state_t s)
{
    int i, j, f;
    memset(user_list, 0, user_devs_count*sizeof(ulong));
    for (i = f = 0; i < root_devs_count; ++i, f = 0) {
        // f of found
        for (j = 0; j < user_devs_count && !f; ++j) {
            if (root_devs[i].serial == user_devs[j].serial) {
                user_list[j] = root_list[i];
                f = 1;
            }
        }
        if (f) {
            debug("Received D%d: %lu", i, root_list[i]);
        } else {
            debug("Received D%d: %lu (ignored)", i, root_list[i]);
        }
    }
    return s;
}

state_t mgt_gpu_eard_freq_limit_get_current(ctx_t *c, ulong *khz)
{
    ulong aux[128]; // khz
	return shuffle_in(khz, aux, eard_rpc(RPC_MGT_GPU_GET_FREQ_LIMIT_CURRENT, NULL, 0, (char *) aux, root_devs_count*sizeof(ulong)));
}

state_t mgt_gpu_eard_freq_limit_get_default(ctx_t *c, ulong *khz)
{
    memcpy(khz, user_freq_default, user_devs_count*sizeof(ulong));
    #if SHOW_DEBUGS
    int u;
    for (u = 0; u < user_devs_count; ++u) {
        debug("D%d default: %lu KHz", u, khz[u]);
    }
    #endif
    return EAR_SUCCESS;
}

state_t mgt_gpu_eard_freq_limit_get_max(ctx_t *c, ulong *khz)
{
    memcpy(khz, user_freq_max, user_devs_count*sizeof(ulong));
    return EAR_SUCCESS;
}

state_t mgt_gpu_eard_freq_limit_reset(ctx_t *c)
{
    return eard_rpc(RPC_MGT_GPU_RESET_FREQ_LIMIT, NULL, 0, NULL, 0);
}

static char *shuffle_out(ulong *user_list, ulong *root_list)
{
    int i, j, f;

    if (root_devs_count == user_devs_count) {
        debug("Root and user are equal, sending the same list");
        return (char *) user_list;
    }
    // Sending 0 is OK, NVML and ONEAPI APIs control when
    // receiving 0, and do nothing.
    memset(root_list, 0, root_devs_count*sizeof(ulong));
    for (i = f = 0; i < root_devs_count; ++i, f = 0) {
        // f of found
        for (j = 0; j < user_devs_count && !f; ++j) {
            if (root_devs[i].serial == user_devs[j].serial) {
                root_list[i] = user_list[j];
                f = 1;
            }
        }
        if (f) {
            debug("Sending D%d: %lu", i, root_list[i]);
        } else {
            debug("Sending D%d: %lu (added by shuffle)", i, root_list[i]);
        }
    }
    return (char *) root_list;
}

state_t mgt_gpu_eard_freq_limit_set(ctx_t *c, ulong *khz)
{
    ulong aux[128];
    debug("mgt_gpu_eard_freq_limit_set");
    return eard_rpc(RPC_MGT_GPU_SET_FREQ_LIMIT, shuffle_out(khz, aux), root_devs_count*sizeof(ulong), NULL, 0);
}

state_t mgt_gpu_eard_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
    debug("mgt_gpu_eard_freq_list");
    *list_khz = (const ulong **) user_freqs_available;
    *list_len = (const uint *) user_freqs_count;
#if SHOW_DEBUGS
    for (int i = 0; i < user_devs_count; i++) {
        debug("GPU %d avail freqs: %u", i, user_freqs_count[i]);
        for (int j = 0; j < user_freqs_count[i]; i++) {
            debug("freq %d: %lu", j, user_freqs_available[i][j]);
        }
    }
#endif
    return EAR_SUCCESS;
}

state_t mgt_gpu_eard_power_cap_get_current(ctx_t *c, ulong *watts)
{
    ulong aux[128];
	return shuffle_in(watts, aux, eard_rpc(RPC_MGT_GPU_GET_POWER_CAP_CURRENT, NULL, 0, (char *) aux, root_devs_count*sizeof(ulong)));
}

state_t mgt_gpu_eard_power_cap_get_default(ctx_t *c, ulong *watts)
{
    memcpy(watts, user_power_default, user_devs_count*sizeof(ulong));
    return EAR_SUCCESS;
}

state_t mgt_gpu_eard_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
    memcpy(watts_max, user_power_max, user_devs_count*sizeof(ulong));
    memcpy(watts_min, user_power_min, user_devs_count*sizeof(ulong));
    return EAR_SUCCESS;
}

state_t mgt_gpu_eard_power_cap_reset(ctx_t *c)
{
    return eard_rpc(RPC_MGT_GPU_RESET_POWER_CAP, NULL, 0, NULL, 0);
}

state_t mgt_gpu_eard_power_cap_set(ctx_t *c, ulong *watts)
{
    ulong aux[128];
	return eard_rpc(RPC_MGT_GPU_SET_POWER_CAP, shuffle_out(watts, aux), root_devs_count*sizeof(ulong), NULL, 0);
}

int mgt_gpu_eard_is_supported()
{
    static int supported = USE_GPUS;
    static int called = 0;
    // This is an independent function. It works always, with independence of the load
    // of the EARD GPU API, although is using RPCs to the EAR Daemon.
    if (!eard || !eards_connected()) {
        return supported;
    }
    if (!called) {
        if (state_fail(eard_rpc(RPC_MGT_GPU_IS_SUPPORTED, NULL, 0, (char *) &supported, sizeof(int)))) {
            debug("Failed to retrieve if GPUs are supported, getting config value");
            supported = USE_GPUS;
        }
        called = 1;
    }
    debug("GPUs supported: %d (USE_GPUS %d)", supported, USE_GPUS);
    return supported;
}
