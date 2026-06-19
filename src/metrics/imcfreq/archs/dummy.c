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
#include <metrics/imcfreq/archs/dummy.h>
#include <stdio.h>

static imcfreq_ops_t *ops_static;
static uint devs_count;

IMCFREQ_F_LOAD(dummy)
{
    // Static info
    devs_count = tp_in->socket_count;
    ops_static = ops;
    // Filling each gap
    apis_put(ops->unload, imcfreq_dummy_unload);
    apis_put(ops->get_info, imcfreq_dummy_get_info);
    apis_put(ops->read, imcfreq_dummy_read);
}

IMCFREQ_F_UNLOAD(dummy)
{
}

IMCFREQ_F_GET_INFO(dummy)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = devs_count;
}

IMCFREQ_F_READ(dummy)
{
    uint cpu;
    // Cleaning
    memset(list, 0, devs_count * sizeof(imcfreq_t));
    for (cpu = 0; cpu < devs_count; ++cpu) {
        list[cpu].error = 1;
    }
    return EAR_SUCCESS;
}