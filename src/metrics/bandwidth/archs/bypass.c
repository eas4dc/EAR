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

//#define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <metrics/cache/cache.h>
#include <metrics/bandwidth/archs/bypass.h>

static uint devs_count;
static uint cpus_count;

state_t bwidth_bypass_load(topology_t *tp, bwidth_ops_t *ops)
{
	state_t s;
	return EAR_ERROR;
	// Pending: test if is something loaded
	if (state_fail(s = cache_load(tp, NO_EARD))) {
		return s;
	}
	cpus_count = tp->cpu_count;
	devs_count = 1;
	// It is shared by all Intel's architecture from Haswell to Skylake
	// In the future it can be initialized in read only mode
	replace_ops(ops->init,            bwidth_bypass_init);
	replace_ops(ops->dispose,         bwidth_bypass_dispose);
	replace_ops(ops->count_devices,   bwidth_bypass_count_devices);
	replace_ops(ops->get_granularity, bwidth_bypass_get_granularity);
	replace_ops(ops->read,            bwidth_bypass_read);

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
	*devs_count_in = devs_count+1;
	return EAR_SUCCESS;
}

state_t bwidth_bypass_get_granularity(ctx_t *c, uint *granularity)
{
	*granularity = GRANULARITY_PROCESS;
	return EAR_SUCCESS;
}

state_t bwidth_bypass_read(ctx_t *c, bwidth_t *bw)
{
	cache_t data;
	state_t s;
	s = cache_read(no_ctx, &data);
	bw[0].cas = data.l3_misses * cpus_count;
	bw[1].time = data.time;
	return s;
}
