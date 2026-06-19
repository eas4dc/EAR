/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* clang-format off */
// #define SHOW_DEBUGS 1
#include <string.h>
#include <common/output/debug.h>
#include <metrics/cache/archs/dummy.h>

CACHE_F_LOAD(dummy)
{
    apis_put(ops->unload  , cache_dummy_unload  );
    apis_put(ops->update  , cache_dummy_update  );
    apis_put(ops->get_info, cache_dummy_get_info);
    apis_put(ops->read    , cache_dummy_read    );
    apis_put(ops->internals_tostr, cache_dummy_internals_tostr);
    debug("Loaded DUMMY");
}

CACHE_F_UNLOAD(dummy)
{
}

CACHE_F_UPDATE(dummy)
{
    return EAR_SUCCESS;
}

CACHE_F_GET_INFO(dummy)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_DUMMY;
    info->granularity = GRANULARITY_PROCESS;
    info->devs_count  = 1;
}

CACHE_F_READ(dummy)
{
    debug("DUMMY READ");
    memset(ca, 0, sizeof(cache_t));
    ca->ll  = &ca->l1d;
    ca->lbw = &ca->l1d;
    return EAR_SUCCESS;
}

CACHE_F_INTERNALS(dummy)
{
    sprintf(buffer, "CACHE LX  : loaded event DUMMY\n");
}