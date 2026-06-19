/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
//#define SHOW_DEBUGS 1
#include <stdlib.h>
#include <common/output/debug.h>
#include <metrics/io/archs/dummy.h>

IO_F_LOAD(dummy)
{
    apis_put(ops->unload    , io_dummy_unload);
    apis_put(ops->update    , io_dummy_update);
    apis_put(ops->get_info  , io_dummy_get_info);
    apis_put(ops->read      , io_dummy_read);
}

IO_F_UNLOAD(dummy)
{
}

IO_F_UPDATE(dummy)
{
    return EAR_SUCCESS;
}

IO_F_GET_INFO(dummy)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_DUMMY;
    info->granularity = GRANULARITY_PROCESS;
    info->devs_count  = 1;
}

IO_F_READ(dummy)
{
    memset(io, 0, sizeof(io_t));
    io[0].pid = 1;
    return EAR_SUCCESS;
}