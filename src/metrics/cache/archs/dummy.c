/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <string.h>
#include <common/output/debug.h>
#include <metrics/cache/archs/dummy.h>

void cache_dummy_load(topology_t *tp, cache_ops_t *ops)
{
    apis_put(ops->get_info,      cache_dummy_get_info);
    apis_put(ops->init,          cache_dummy_init);
    apis_put(ops->dispose,       cache_dummy_dispose);
    apis_put(ops->read,          cache_dummy_read);
    apis_put(ops->data_diff,     cache_dummy_data_diff);
    apis_put(ops->internals_tostr, cache_dummy_internals_tostr);
    debug("Loaded DUMMY");
}

void cache_dummy_get_info(apinfo_t *info)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_PROCESS;
    info->granularity = GRANULARITY_PROCESS;
    info->devs_count  = 1;
    info->bits        = 0;
}

state_t cache_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cache_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cache_dummy_read(ctx_t *c, cache_t *ca)
{
    debug("DUMMY READ");
	memset(ca, 0, sizeof(cache_t));
	return EAR_SUCCESS;
}

void cache_dummy_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
    memset(caD, 0, sizeof(cache_t));
    if (gbs != NULL) {
        *gbs = 0.0;
    }
}

void cache_dummy_internals_tostr(char *buffer, int length)
{
    sprintf(buffer, "CACHE LX  : loaded event DUMMY\n");
}