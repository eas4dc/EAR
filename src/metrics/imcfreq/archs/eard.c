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
#include <daemon/local_api/eard_api.h>
#include <metrics/imcfreq/archs/eard.h>

static uint devs_count;
static uint eard_api;

IMCFREQ_F_LOAD(eard)
{
    state_t s;

    if (!API_IS(options, API_EARD)) {
        return;
    }
    debug("connecting with daemon");
    if (!eards_connected()) {
        debug("EARD is not running or EAR_TMP is not set correctly");
        return;
    }
    // Contacting daemon
    if (state_fail(s = eard_rpc(RPC_MET_IMCFREQ_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
        debug("RPC RPC_MET_CPUFREQ_GET_API returned: %s (%d)", state_msg, s);
        return;
    }
    if (state_fail(s = eard_rpc(RPC_MET_IMCFREQ_COUNT_DEVICES, NULL, 0, (char *) &devs_count, sizeof(uint)))) {
        debug("RPC RPC_MET_IMCFREQ_COUNT_DEVICES returned: %s (%d)", state_msg, s);
        return;
    }
    if (eard_api == API_NONE || eard_api == API_DUMMY) {
        debug("EARD has loaded DUMMY API");
        return;
    }
    if (!devs_count) {
        return;
    }
    apis_put(ops->unload, imcfreq_eard_unload);
    apis_put(ops->get_info, imcfreq_eard_get_info);
    apis_put(ops->read, imcfreq_eard_read);
    debug("EARD loaded full API");
}

IMCFREQ_F_UNLOAD(eard)
{
}

IMCFREQ_F_GET_INFO(eard)
{
    info->api         = API_EARD;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = devs_count;
}

IMCFREQ_F_READ(eard)
{
    memset((void *) list, 0, sizeof(imcfreq_t) * devs_count);
    return eard_rpc(RPC_MET_IMCFREQ_GET_CURRENT, NULL, 0, (char *) list, sizeof(imcfreq_t) * devs_count);
}
