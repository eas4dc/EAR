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
#include <errno.h>
#include <metrics/temperature/archs/dummy.h>
#include <stdlib.h>

static uint socket_count;

TEMP_F_LOAD(dummy)
{
    socket_count = (tp->socket_count > 0) ? tp->socket_count : 1;
    apis_put(ops->get_info, temp_dummy_get_info);
    apis_put(ops->init, temp_dummy_init);
    apis_put(ops->dispose, temp_dummy_dispose);
    apis_put(ops->read, temp_dummy_read);
}

TEMP_F_GET_INFO(dummy)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = socket_count;
}

TEMP_F_INIT(dummy)
{
    return EAR_SUCCESS;
}

TEMP_F_DISPOSE(dummy)
{
    // Empty
}

TEMP_F_READ(dummy)
{
    debug("temp dummy read");
    if (temp != NULL) {
        memset((void *) temp, 0, sizeof(llong) * socket_count);
    }
    if (average != NULL) {
        *average = 0;
    }
    return EAR_SUCCESS;
}
