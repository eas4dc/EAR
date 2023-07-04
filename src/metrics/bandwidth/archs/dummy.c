/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <stdlib.h>
#include <metrics/bandwidth/archs/dummy.h>

BWIDTH_F_LOAD(bwidth_dummy_load)
{
    apis_put(ops->get_info, bwidth_dummy_get_info);
    apis_put(ops->init,     bwidth_dummy_init);
    apis_put(ops->dispose,  bwidth_dummy_dispose);
    apis_put(ops->read,     bwidth_dummy_read);
}

BWIDTH_F_GET_INFO(bwidth_dummy_get_info)
{
    if (info->api == API_NONE) {
        info->api         = API_DUMMY;
        info->scope       = SCOPE_NODE;
        info->granularity = GRANULARITY_DUMMY;
        info->devs_count  = 1+1;
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
	memset(bws, 0, (1+1)*sizeof(bwidth_t));
	return EAR_SUCCESS;
}
