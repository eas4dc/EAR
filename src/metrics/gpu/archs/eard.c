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
// #define FAKE_GPUS   1

#include <common/config/config_install.h>
#include <common/output/debug.h>
#include <common/utils/serial_buffer.h>
#include <daemon/local_api/eard_api_rpc.h>
#include <metrics/gpu/archs/eard.h>
#include <stdlib.h>

static int eard;
static uint root_devs_count = 0;
static uint user_devs_count = 0;
static gpu_devs_t *root_devs;
static gpu_devs_t *user_devs;
static uint eard_api;

void gpu_eard_load(gpu_ops_t *ops, int _eard)
{
    wide_buffer_t b;
    state_t s;

    eard = _eard;
    if (!eard) {
        debug("EARD (daemon) not required");
        return;
    }
    if (!eards_connected()) {
        debug("EARD (daemon) not connected");
        return;
    }
    debug("EARD (daemon) is connected");
    if (state_fail(s = eard_rpc(RPC_MET_GPU_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
        debug("Received: %s", state_msg) return;
    }
    if (eard_api == API_NONE || eard_api == API_DUMMY) {
        debug("EARD (daemon) has loaded DUMMY/NONE API");
        return;
    }
    // Get a list of devices in root space
    if (state_fail(s = eard_rpc_buffered(RPC_MET_GPU_GET_DEVICES, NULL, 0, (char **) &b, NULL))) {
        return;
    }
    serial_copy_elem(&b, (char *) &root_devs_count, NULL);
    debug("Received from EARD %u GPUs", root_devs_count);
    if (root_devs_count == 0) {
        debug("There are no GPUs");
        return;
    }
    root_devs       = (gpu_devs_t *) serial_copy_elem(&b, NULL, NULL);
    user_devs_count = root_devs_count;
    // Get a list of devices in user space
    if (ops->get_devices != NULL) {
        ops->get_devices(&user_devs, &user_devs_count);
    } else {
        // If not, just copy root devices
        user_devs = calloc(user_devs_count, sizeof(gpu_devs_t));
        memcpy(user_devs, root_devs, user_devs_count * sizeof(gpu_devs_t));
    }
#if FAKE_GPUS
    user_devs_count = 1;
#endif
#if SHOW_DEBUGS
    int i;

    for (i = 0; i < root_devs_count; ++i) {
        debug("root D%d: %d_%llu", i, root_devs[i].index, root_devs[i].serial);
    }
    for (i = 0; i < user_devs_count; ++i) {
        debug("user D%d: %d_%llu", i, user_devs[i].index, user_devs[i].serial);
    }
#endif
    // If something already loaded
    apis_put(ops->get_info, gpu_eard_get_info);
    apis_put(ops->get_devices, gpu_eard_get_devices);
    apis_put(ops->init, gpu_eard_init);
    apis_put(ops->dispose, gpu_eard_dispose);
    apis_set(ops->read, gpu_eard_read);
    apis_set(ops->read_raw, gpu_eard_read_raw);
    debug("Loaded metrics/gpu/EARD");
}

void gpu_eard_get_info(apinfo_t *info)
{
    info->api        = API_EARD;
    info->api_under  = eard_api;
    info->devs_count = user_devs_count;
}

void gpu_eard_get_devices(gpu_devs_t **devs, uint *devs_count)
{
    if (devs != NULL) {
        *devs = calloc(user_devs_count, sizeof(gpu_devs_t));
        memcpy(*devs, user_devs, user_devs_count * sizeof(gpu_devs_t));
    }
    if (devs_count != NULL) {
        *devs_count = (uint) user_devs_count;
    }
}

state_t gpu_eard_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t gpu_eard_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

static state_t static_read(uint call, gpu_t *data)
{
    size_t size = ((size_t) root_devs_count) * sizeof(gpu_t);
    gpu_t root_data[MAX_GPUS_SUPPORTED];
    int u, r, f;
    state_t s;

    memset((char *) root_data, 0, size);
    if (state_fail(s = eard_rpc(call, NULL, 0, (char *) root_data, size))) {
        return s;
    }
    for (r = f = 0; r < root_devs_count; ++r, f = 0) {
        for (u = 0; u < user_devs_count; ++u) {
            if (user_devs[u].serial == root_devs[r].serial) {
                data[u] = root_data[r];
                f       = 1;
            }
        }
        if (f) {
            debug("Received D%d", r);
        } else {
            debug("Received D%d (ignored)", r);
        }
    }
    return EAR_SUCCESS;
}

state_t gpu_eard_read(ctx_t *c, gpu_t *data)
{
    state_t s = static_read(RPC_MET_GPU_GET_METRICS, data);
#if SHOW_DEBUGS
    gpu_data_print(data, debug_channel);
#endif
    return s;
}

state_t gpu_eard_read_raw(ctx_t *c, gpu_t *data)
{
    return static_read(RPC_MET_GPU_GET_METRICS_RAW, data);
}

int gpu_eard_is_supported()
{
    static int supported = USE_GPUS;
    static int called    = 0;
    // This is an independent function. It works always, with independence of the load
    // of the EARD GPU API, although is using RPCs to the EAR Daemon.
    if (!eard || !eards_connected()) {
#if FAKE_EAR_NOT_INSTALLED
        verbose(0, " FAKE_EAR_NOT_INSTALLED supported %u", supported);
#endif
        return supported;
    }
    if (!called) {
        if (state_fail(eard_rpc(RPC_MET_GPU_IS_SUPPORTED, NULL, 0, (char *) &supported, sizeof(int)))) {
            debug("Failed to retrieve if GPUs are supported, getting config value");
            supported = USE_GPUS;
        }
        called = 1;
    }
    debug("GPUs supported: %d (USE_GPUS %d)", supported, USE_GPUS);
    return supported;
}
