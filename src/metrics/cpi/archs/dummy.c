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
#include <metrics/cpi/archs/dummy.h>

CPI_F_LOAD(dummy)
{
    apis_put(ops->unload    , cpi_dummy_unload  );
    apis_put(ops->update,     cpi_dummy_update  );
    apis_put(ops->get_info  , cpi_dummy_get_info);
    apis_put(ops->read      , cpi_dummy_read    );
}

CPI_F_UNLOAD(dummy)
{
}

CPI_F_UPDATE(dummy)
{
    return EAR_SUCCESS;
}

CPI_F_GET_INFO(dummy)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_DUMMY;
    info->granularity = GRANULARITY_PROCESS;
    info->devs_count  = 1;
}

CPI_F_READ(dummy)
{
    memset(cpi, 0, sizeof(cpi_t));
    return EAR_SUCCESS;
}