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

#include <common/hardware/topology.h>
#include <common/output/debug.h>
#include <daemon/local_api/eard_api.h>
#include <metrics/energy_cpu/archs/eard.h>
#include <stdlib.h>

topology_t tp;

state_t energy_cpu_eard_load(topology_t *tp_in, uint eard)
{
    debug("entering eard_load with eard_required %u", eard);
    if (!eard) {
        return_msg(EAR_ERROR, "EARD (daemon) not required");
    }
    if (!eards_connected()) {
        return_msg(EAR_ERROR, "EARD (daemon) not connected");
    }

    state_t s = eards_get_data_size_rapl(); // to internally store the rapl_size in the local_api

    if (state_fail(s)) {
        debug("eards_get_data_size returns %u", s);
    }

    memcpy(&tp, tp_in, sizeof(topology_t));

    return EAR_SUCCESS;
}

state_t energy_cpu_eard_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t energy_cpu_eard_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t energy_cpu_eard_count_devices(ctx_t *c, uint *devs_count_in)
{
    *devs_count_in = tp.socket_count;
    return EAR_SUCCESS;
}

state_t energy_cpu_eard_get_granularity(ctx_t *c, uint *granularity_in)
{
    //*granularity_in = GRANULARITY_IMC;
    return EAR_SUCCESS;
}

state_t energy_cpu_eard_read(ctx_t *c, ullong *values)
{
    state_t s = eards_read_rapl(values);
    return s;
}
