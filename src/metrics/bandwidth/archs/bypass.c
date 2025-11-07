/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <metrics/bandwidth/archs/bypass.h>
#include <metrics/cache/cache.h>

static uint devs_count;
static uint cpus_count;

state_t bwidth_bypass_load(topology_t *tp, bwidth_ops_t *ops)
{
    return EAR_ERROR;
#if 0
	state_t s;
	// Pending: test if is something loaded
	if (state_fail(s = cache_load(tp, NO_EARD))) {
		return s;
	}
#endif
    cpus_count = tp->cpu_count;
    devs_count = 1;
    // It is shared by all Intel's architecture from Haswell to Skylake
    // In the future it can be initialized in read only mode
    replace_ops(ops->init, bwidth_bypass_init);
    replace_ops(ops->dispose, bwidth_bypass_dispose);
    replace_ops(ops->count_devices, bwidth_bypass_count_devices);
    replace_ops(ops->read, bwidth_bypass_read);

    return EAR_SUCCESS;
}

state_t bwidth_bypass_init(ctx_t *c)
{
    return cache_init(no_ctx);
}

state_t bwidth_bypass_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t bwidth_bypass_count_devices(ctx_t *c, uint *devs_count_in)
{
    *devs_count_in = devs_count + 1;
    return EAR_SUCCESS;
}

state_t bwidth_bypass_read(ctx_t *c, bwidth_t *bw)
{
    cache_t data;
    state_t s;
    s          = cache_read(no_ctx, &data);
    bw[0].cas  = (data.lbw->lines_in + data.lbw->lines_out) * cpus_count;
    bw[1].time = data.time;
    return s;
}
