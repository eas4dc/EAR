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

#include <common/config/config_install.h>
#include <common/output/debug.h>
#include <common/utils/serial_buffer.h>
#include <daemon/local_api/eard_api_rpc.h>
#include <metrics/gpu/archs/eard.h>
#include <stdlib.h>

static uint eard_api;
static gpu_topology_t tp_root;
static gpu_topology_t tp_user;
static gpu_topology_t tp_root_rd; // readable devices only
static gpu_topology_t tp_user_rd; // readable devices only

GPU_F_LOAD(eard)
{
    wide_buffer_t b;
    state_t s;

    // We don't test if an API is already loaded because EARD option prevents
    // GPU pooling.
    if (!API_IS(options, API_EARD)) {
        debug("EARD (daemon) not required");
        return;
    }
    if (!eards_connected()) {
        debug("EARD (daemon) not connected");
        return;
    }
    debug("EARD (daemon) is connected");
    if (state_fail(s = eard_rpc(RPC_MET_GPU_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
        debug("Bad reception: %s", state_msg) return;
        return;
    }
    if (eard_api == API_NONE || eard_api == API_DUMMY) {
        debug("EARD (daemon) has loaded DUMMY/NONE API");
        return;
    }
    // Get a list of devices in root space
    if (state_fail(s = eard_rpc_buffered(RPC_MET_GPU_TOPOLOGY_GET, NULL, 0, (char **) &b, NULL))) {
        debug("Bad reception: %s", state_msg);
        return;
    }
    serial_copy_elem(&b, (char *) &tp_root, NULL);
    debug("Received from EARD a topology of %u GPUs", tp_root.devs_count);
    if (tp_root.devs_count == 0) {
        debug("There are no GPUs");
        return;
    }
    tp_root.devs = (gpu_devs_t *) serial_copy_elem(&b, NULL, NULL);
    // Getting the user part
    tp_user.devs_count = tp_root.devs_count;
    // Get a list of devices in user space
    if (ops->topology_get != NULL) {
        ops->topology_get(&tp_user);
    } else {
        // If not, just copy root devices
        tp_user.devs = calloc(tp_user.devs_count, sizeof(gpu_devs_t));
        memcpy(tp_user.devs, tp_root.devs, tp_user.devs_count * sizeof(gpu_devs_t));
    }
    gpu_topology_select(&tp_root, &tp_root_rd, GPU_TP_SEL_READABLE);
    gpu_topology_select(&tp_user, &tp_user_rd, GPU_TP_SEL_READABLE);
    debug("TP root (ALL) has %d devs", tp_root.devs_count);
    debug("TP root (RD ) has %d devs", tp_root_rd.devs_count);
    debug("TP user (ALL) has %d devs", tp_user.devs_count);
    debug("TP user (RD ) has %d devs", tp_user_rd.devs_count);
    // If something already loaded
    apis_put(ops->unload, gpu_eard_unload);
    apis_put(ops->get_info, gpu_eard_get_info);
    apis_put(ops->topology_get, gpu_eard_topology_get);
    apis_set(ops->read, gpu_eard_read);
    apis_set(ops->read_raw, gpu_eard_read_raw);
    debug("Loaded EARD");
}

GPU_F_UNLOAD(eard)
{
    gpu_topology_free(&tp_root);
    gpu_topology_free(&tp_user);
    gpu_topology_free(&tp_root_rd);
    gpu_topology_free(&tp_user_rd);
}

GPU_F_GET_INFO(eard)
{
    info->api        = API_EARD;
    info->api_under  = eard_api;
    info->devs_count = tp_user_rd.devs_count;
}

GPU_F_TOPOLOGY_GET(eard)
{
    tp->devs_count = tp_user.devs_count;
    tp->devs       = calloc(tp_user.devs_count, sizeof(gpu_devs_t));
    memcpy(tp->devs, tp_user.devs, tp_user.devs_count * sizeof(gpu_devs_t));
}

static state_t static_read(uint call, gpu_t *data)
{
    size_t size = ((size_t) tp_root_rd.devs_count) * sizeof(gpu_t);
    gpu_t root_data[MAX_GPUS_SUPPORTED];
    int u, r, f;
    state_t s;

    memset((char *) root_data, 0, size);
    if (state_fail(s = eard_rpc(call, NULL, 0, (char *) root_data, size))) {
        return s;
    }
    for (r = f = 0; r < tp_root_rd.devs_count; ++r, f = 0) {
        for (u = 0; u < tp_user_rd.devs_count; ++u) {
            if (tp_user_rd.devs[u].serial == tp_root_rd.devs[r].serial) {
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

GPU_F_READ(eard)
{
    state_t s = static_read(RPC_MET_GPU_GET_METRICS, d);
#if SHOW_DEBUGS
    gpu_data_print(d, debug_channel);
#endif
    return s;
}

GPU_F_READ_RAW(eard)
{
    return static_read(RPC_MET_GPU_GET_METRICS_RAW, d);
}

int gpu_eard_is_supported()
{
    static int supported = USE_GPUS;
    static int called    = 0;
    // This is an independent function. It works always, with independence of the load
    // of the EARD GPU API, although is using RPCs to the EAR Daemon.
    if (!eard_api || !eards_connected()) {
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