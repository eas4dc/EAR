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
#include <metrics/flops/archs/dummy.h>

void flops_dummy_load(topology_t *tp, flops_ops_t *ops)
{
    apis_put(ops->get_info,        flops_dummy_get_info);
    apis_put(ops->init,            flops_dummy_init);
    apis_put(ops->dispose,         flops_dummy_dispose);
    apis_put(ops->read,            flops_dummy_read);
    apis_put(ops->data_diff,       flops_dummy_data_diff);
    apis_put(ops->internals_tostr, flops_dummy_internals_tostr);
}

void flops_dummy_get_info(apinfo_t *info)
{
    info->api         = API_DUMMY;
    info->scope       = SCOPE_PROCESS;
    info->granularity = GRANULARITY_PROCESS;
    info->devs_count  = 1;
    info->bits        = 0;
}

state_t flops_dummy_init()
{
	return EAR_SUCCESS;
}

state_t flops_dummy_dispose()
{
	return EAR_SUCCESS;
}

state_t flops_dummy_read(flops_t *fl)
{
	memset(fl, 0, sizeof(flops_t));
	return EAR_SUCCESS;
}

void flops_dummy_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs)
{
    // Cleaning
    if (gfs != NULL) { *gfs = 0.0; }
    if (flD != NULL) { memset(flD, 0, sizeof(flops_t)); }
}

void flops_dummy_internals_tostr(char *buffer, int length)
{
    sprintf(buffer, "FLOPS X86 : loaded event DUMMY\n");
}