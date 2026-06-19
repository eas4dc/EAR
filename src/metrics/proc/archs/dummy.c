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
// #define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <metrics/proc/archs/dummy.h>

PROC_F_LOAD(dummy)
{
    apis_put(ops->unload    , proc_dummy_unload  );
    apis_put(ops->update    , proc_dummy_update  );
    apis_put(ops->get_info  , proc_dummy_get_info);
    apis_put(ops->read      , proc_dummy_read    );
}

PROC_F_UNLOAD(dummy)
{
}

PROC_F_UPDATE(dummy)
{
    return EAR_SUCCESS;
}

PROC_F_GET_INFO(dummy)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_DUMMY;
    info->granularity = GRANULARITY_PROCESS;
    info->devs_count  = 1;
}

PROC_F_READ(dummy)
{
    memset(pr, 0, sizeof(proc_t));
    return EAR_SUCCESS;
}