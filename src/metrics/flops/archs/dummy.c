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
#include <string.h>
#include <common/output/debug.h>
#include <metrics/flops/archs/dummy.h>

FLOPS_F_LOAD(dummy)
{
    apis_put(ops->unload   , flops_dummy_unload);
    apis_put(ops->update   , flops_dummy_update);
    apis_put(ops->get_info , flops_dummy_get_info);
    apis_put(ops->read     , flops_dummy_read);
    apis_put(ops->data_diff, flops_dummy_data_diff);
    apis_put(ops->internals_tostr, flops_dummy_internals_tostr);
}

FLOPS_F_UNLOAD(dummy)
{
}

FLOPS_F_UPDATE(dummy)
{
    return EAR_SUCCESS;
}

FLOPS_F_GET_INFO(dummy)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_DUMMY;
    info->granularity = GRANULARITY_PROCESS;
    info->devs_count  = 1;
}

FLOPS_F_READ(dummy)
{
    memset(fl, 0, sizeof(flops_t));
    return EAR_SUCCESS;
}

FLOPS_F_DATA_DIFF(dummy)
{
    // Cleaning
    if (gfs != NULL) {
        *gfs = 0.0;
    }
    if (flD != NULL) {
        memset(flD, 0, sizeof(flops_t));
    }
}

FLOPS_F_INTERNALS_TOSTR(dummy)
{
    sprintf(buffer, "FLOPS X86 : loaded event DUMMY\n");
}