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
#include <daemon/local_api/eard_api_rpc.h>
#include <metrics/bandwidth/archs/eard.h>
#include <stdlib.h>

static uint devs_count;

#define RPC_GET_API       RPC_MET_BWIDTH_GET_API
#define RPC_COUNT_DEVICES RPC_MET_BWIDTH_COUNT_DEVICES
#define RPC_READ          RPC_MET_BWIDTH_READ

state_t bwidth_eard_load(topology_t *tp, bwidth_ops_t *ops, uint eard)
{
    uint eard_api;
    state_t s;

    if (!eard) {
        debug("EARD (daemon) not required");
        return_msg(EAR_ERROR, "EARD (daemon) not required");
    }
    if (!eards_connected()) {
        debug("EARD (daemon) not connected");
        return_msg(EAR_ERROR, "EARD (daemon) not connected");
    }
    // Get API
    if (state_fail(s = eard_rpc(RPC_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
        return s;
    }
    if (eard_api == API_NONE || eard_api == API_DUMMY) {
        debug("EARD (daemon) has loaded DUMMY/NONE API");
        return_msg(EAR_ERROR, "EARD (daemon) has loaded DUMMY/NONE API");
    }
    // Get devices
    if (state_fail(s = eard_rpc(RPC_COUNT_DEVICES, NULL, 0, (char *) &devs_count, sizeof(uint)))) {
        return s;
    }
    debug("Remote #devices: %u", devs_count);
    apis_put(ops->get_info, bwidth_eard_get_info);
    apis_put(ops->init, bwidth_eard_init);
    apis_put(ops->dispose, bwidth_eard_dispose);
    apis_put(ops->read, bwidth_eard_read);
    debug("Loaded EARD");
    return EAR_SUCCESS;
}

BWIDTH_F_GET_INFO(bwidth_eard_get_info)
{
    info->api = API_EARD;
    // Granularity and scope are invented by now
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_IMC;
    // EARD doesn't have to add 1
    info->devs_count = devs_count;
}

state_t bwidth_eard_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t bwidth_eard_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t bwidth_eard_count_devices(ctx_t *c, uint *devs_count_in)
{
    *devs_count_in = devs_count;
    return EAR_SUCCESS;
}

state_t bwidth_eard_read(ctx_t *c, bwidth_t *b)
{
    state_t s = eard_rpc(RPC_READ, NULL, 0, (char *) b, sizeof(bwidth_t) * devs_count);
    debug("Received CAS0 %llu", b[0].cas);
    return s;
}
