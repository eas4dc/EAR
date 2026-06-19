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
#include <metrics/cpufreq/archs/eard.h>
#include <stdlib.h>

static uint cpus_count;

CPUFREQ_F_LOAD(eard)
{
    uint eard_api;
    state_t s;

    if (ops->read != NULL) {
        return;
    }
    if (!API_IS(options, API_EARD)) {
        return;
    }
    if (!eards_connected()) {
        return_msg(, "EARD (daemon) not connected");
    }
    if (state_fail(s = eard_rpc(RPC_MET_CPUFREQ_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
        debug("RPC RPC_MET_CPUFREQ_GET_API returned: %s (%d)", state_msg, s);
        return;
    }
    if (eard_api == API_NONE || eard_api == API_DUMMY) {
        return_msg(, "EARD (daemon) has loaded DUMMY/NONE API");
    }
    cpus_count = tp->cpu_count;
    apis_put(ops->unload, cpufreq_eard_unload);
    apis_put(ops->get_info, cpufreq_eard_get_info);
    apis_put(ops->read, cpufreq_eard_read);
}

CPUFREQ_F_UNLOAD(eard)
{
}

CPUFREQ_F_GET_INFO(eard)
{
    info->api         = API_EARD;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_CPU;
    info->devs_count  = cpus_count;
}

CPUFREQ_F_READ(eard)
{
    debug("cpufreq_eard_read");
    memset((void *) f, 0, sizeof(cpufreq_t) * cpus_count);
    return eard_rpc(RPC_MET_CPUFREQ_GET_CURRENT, NULL, 0, (char *) f, sizeof(cpufreq_t) * cpus_count);
}
