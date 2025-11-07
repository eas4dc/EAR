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

static uint cpu_count;

state_t cpufreq_eard_status(topology_t *tp, cpufreq_ops_t *ops, int eard)
{
    uint eard_api;
    state_t s;

    if (!eard) {
        return_msg(EAR_ERROR, "EARD (daemon) not required");
    }
    if (!eards_connected()) {
        return_msg(EAR_ERROR, "EARD (daemon) not connected");
    }
    if (state_fail(s = eard_rpc(RPC_MET_CPUFREQ_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
        debug("RPC RPC_MET_CPUFREQ_GET_API returned: %s (%d)", state_msg, s);
        return s;
    }
    if (eard_api == API_NONE || eard_api == API_DUMMY) {
        return_msg(EAR_ERROR, "EARD (daemon) has loaded DUMMY/NONE API");
    }
    //
    cpu_count = tp->cpu_count;
    //
    replace_ops(ops->read, cpufreq_eard_read);

    return EAR_SUCCESS;
}

state_t cpufreq_eard_read(ctx_t *c, cpufreq_t *list)
{
    debug("cpufreq_eard_read");
    memset((void *) list, 0, sizeof(cpufreq_t) * cpu_count);
    return eard_rpc(RPC_MET_CPUFREQ_GET_CURRENT, NULL, 0, (char *) list, sizeof(cpufreq_t) * cpu_count);
}
