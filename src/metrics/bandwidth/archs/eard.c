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
static uint eard_api;

#define RPC_GET_API       RPC_MET_BWIDTH_GET_API
#define RPC_COUNT_DEVICES RPC_MET_BWIDTH_COUNT_DEVICES
#define RPC_READ          RPC_MET_BWIDTH_READ

BWIDTH_F_LOAD(eard)
{
    state_t s;

    if (ops->read != NULL) {
        return;
    }
    if (!API_IS(options, API_EARD)) {
        debug("EARD (daemon) not required");
        return;
    }
    if (!eards_connected()) {
        debug("EARD (daemon) not connected");
        return;
    }
    // Get API
    if (state_fail(s = eard_rpc(RPC_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
        return;
    }
    if (eard_api == API_NONE || eard_api == API_DUMMY) {
        debug("EARD (daemon) has loaded DUMMY/NONE API");
        return;
    }
    // Get devices
    if (state_fail(s = eard_rpc(RPC_COUNT_DEVICES, NULL, 0, (char *) &devs_count, sizeof(uint)))) {
        return;
    }
    debug("Remote #devices: %u", devs_count);
    apis_put(ops->unload, bwidth_eard_unload);
    apis_put(ops->get_info, bwidth_eard_get_info);
    apis_put(ops->read, bwidth_eard_read);
    debug("Loaded EARD");
}

BWIDTH_F_UNLOAD(eard)
{
}

BWIDTH_F_GET_INFO(eard)
{
    info->api         = API_EARD;
    info->api_under   = eard_api;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_IMC;
    info->devs_count  = devs_count;
}

BWIDTH_F_READ(eard)
{
    state_t s = eard_rpc(RPC_READ, NULL, 0, (char *) b, sizeof(bwidth_t) * devs_count);
    debug("Received CAS0 %llu", b[0].cas);
    return s;
}