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
#include <metrics/common/apis.h>
#include <metrics/temperature/archs/eard.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static apinfo_t self_info;
static llong *read_values;

TEMP_F_LOAD(eard)
{
    state_t s;

    // Another API is opened and this API is not mixtable
    if (ops->read != NULL) {
        return;
    }
    if (force_api != API_EARD) {
        return;
    }
    debug("Trying to connect to EARD");
    if (!eards_connected()) {
        return;
    }
    debug("Connected to EARD");
    if (state_fail(s = eard_rpc(RPC_MET_TEMP_GET_INFO, NULL, 0, (char *) &self_info, sizeof(apinfo_t)))) {
        return;
    }
    debug("Transmitted API %d and #devs %d", self_info.api, self_info.devs_count);
    if (self_info.api == API_NONE || self_info.api == API_DUMMY || self_info.devs_count == 0) {
        return;
    }
    read_values = calloc(self_info.devs_count, sizeof(llong));
    apis_set(ops->get_info, temp_eard_get_info);
    apis_set(ops->init, temp_eard_init);
    apis_set(ops->dispose, temp_eard_dispose);
    apis_set(ops->read, temp_eard_read);
}

TEMP_F_GET_INFO(eard)
{
    info->api         = API_EARD;
    info->scope       = self_info.scope;
    info->granularity = self_info.granularity;
    info->devs_count  = self_info.devs_count;
}

TEMP_F_INIT(eard)
{
    return EAR_SUCCESS;
}

TEMP_F_DISPOSE(eard)
{
    // Empty
}

TEMP_F_READ(eard)
{
    state_t s;
    int i;

    if (state_fail(
            s = eard_rpc(RPC_MET_TEMP_READ, NULL, 0, (char *) read_values, self_info.devs_count * sizeof(llong)))) {
        return s;
    }
    // Calculating the average
    if (temp != NULL) {
        memcpy(temp, read_values, self_info.devs_count * sizeof(llong));
    }
    if (average != NULL) {
        for (i = 0; i < self_info.devs_count; ++i) {
            *average += read_values[i];
        }
        *average /= (llong) self_info.devs_count;
    }
    return s;
}
