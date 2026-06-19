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
#include <common/output/debug.h>
#include <metrics/cache/cache.h>
#include <metrics/bandwidth/archs/bypass.h>

static apinfo_t info;

BWIDTH_F_LOAD(bypass)
{
    // Disabled by now (reason?)
    #if 1
    return;
    #endif
    // If an API is already loaded we must avoid loading cache API.
    if (ops->read != NULL) {
        return;
    }
    // Pending: test if is something loaded
    cache_load(tp, options);
    cache_get_info(&info);

    apis_put(ops->unload  , bwidth_bypass_unload);
    apis_put(ops->get_info, bwidth_bypass_get_info);
    apis_put(ops->read    , bwidth_bypass_read);
}

BWIDTH_F_UNLOAD(bypass)
{
    // Closing and API could be problematic if is used in other places.
    // cache_close();
}

BWIDTH_F_GET_INFO(bypass)
{
    info->api         = API_AMD17;
    info->scope       = info->scope;
    info->granularity = info->granularity;
    info->devs_count  = info->devs_count + 1;
}

BWIDTH_F_READ(bypass)
{
    static cache_t *cache = NULL;
    state_t s;
    uint d;

    if (data == NULL) {
        cache_data_alloc(&cache);
    }
    s = cache_read(&data);
    for (d = 0; s == STATE_OK && d < info.devs_count; ++d) {
        b[0].cas += (cache[d].lbw->lines_in + cache[d].lbw->lines_out);
    }
    b[1].time = (cache[0].time);
    return s;
}
