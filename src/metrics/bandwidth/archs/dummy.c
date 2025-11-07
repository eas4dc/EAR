/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <metrics/bandwidth/archs/dummy.h>
#include <stdlib.h>

BWIDTH_F_LOAD(bwidth_dummy_load)
{
    apis_put(ops->get_info, bwidth_dummy_get_info);
    apis_put(ops->init, bwidth_dummy_init);
    apis_put(ops->dispose, bwidth_dummy_dispose);
    apis_put(ops->read, bwidth_dummy_read);
}

BWIDTH_F_GET_INFO(bwidth_dummy_get_info)
{
    if (info->api == API_NONE) {
        info->api         = API_DUMMY;
        info->scope       = SCOPE_NODE;
        info->granularity = GRANULARITY_DUMMY;
        info->devs_count  = 1 + 1;
    }
}

BWIDTH_F_INIT(bwidth_dummy_init)
{
    return EAR_SUCCESS;
}

BWIDTH_F_DISPOSE(bwidth_dummy_dispose)
{
    return EAR_SUCCESS;
}

BWIDTH_F_READ(bwidth_dummy_read)
{
    memset(bws, 0, (1 + 1) * sizeof(bwidth_t));
    return EAR_SUCCESS;
}
