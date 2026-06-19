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

BWIDTH_F_LOAD(dummy)
{
    apis_put(ops->unload, bwidth_dummy_unload);
    apis_put(ops->get_info, bwidth_dummy_get_info);
    apis_put(ops->read, bwidth_dummy_read);
}

BWIDTH_F_UNLOAD(dummy)
{
}

BWIDTH_F_GET_INFO(dummy)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_DUMMY;
    info->devs_count  = 1 + 1;
}

BWIDTH_F_READ(dummy)
{
    memset(b, 0, (1 + 1) * sizeof(bwidth_t));
    return EAR_SUCCESS;
}
