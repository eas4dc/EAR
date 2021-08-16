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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#include <metrics/cache/cache.h>
#include <metrics/cache/cpu/perf.h>

static struct cache_ops
{
	state_t (*init)			(ctx_t *c);
	state_t (*dispose)		(ctx_t *c);
	state_t (*reset)		(ctx_t *c);
	state_t (*start)		(ctx_t *c);
	state_t (*stop)			(ctx_t *c, llong *L1_misses, llong *LL_misses);
	state_t (*read)			(ctx_t *c, llong *L1_misses, llong *LL_misses);
	state_t (*data_print)	(ctx_t *c, llong  L1_misses, llong  LL_misses);
} ops;

state_t cache_load(topology_t *tp)
{
	if (state_ok(cache_perf_status(tp)))
	{
		ops.init		= cache_perf_init;
		ops.dispose		= cache_perf_dispose;
		ops.reset		= cache_perf_reset;
		ops.start		= cache_perf_start;
		ops.stop		= cache_perf_stop;
		ops.read		= cache_perf_read;
		ops.data_print	= cache_perf_data_print;
	} else {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}

	return EAR_SUCCESS;
}

state_t cache_init()
{
	preturn(ops.init, NULL);
}

state_t cache_dispose()
{
	preturn(ops.dispose, NULL);
}

state_t cache_reset()
{
	preturn(ops.reset, NULL);
}

state_t cache_start()
{
	preturn(ops.start, NULL);
}

/* This is an obsolete function to make metrics.c compatible. */
static long long accum_L1_misses;
static long long accum_LL_misses;

state_t cache_stop(llong *L1_misses, llong *LL_misses)
{
	if (ops.stop != NULL) {
		ops.stop(NULL, L1_misses, LL_misses);
	}
	accum_L1_misses += *L1_misses;
    accum_LL_misses += *LL_misses;
	return EAR_SUCCESS;
}

state_t cache_read(llong *L1_misses, llong *LL_misses)
{
	if (ops.read != NULL) {
		ops.read(NULL, L1_misses, LL_misses);
	}
	accum_L1_misses += *L1_misses;
	accum_LL_misses += *LL_misses;
	return EAR_SUCCESS;
}

state_t cache_data_print(llong L1_misses, llong LL_misses)
{
	preturn(ops.data_print, NULL, L1_misses, LL_misses);
}

void get_cache_metrics(llong *L1_misses, llong *LL_misses)
{
	cache_read(L1_misses, LL_misses);
	*L1_misses += accum_L1_misses;
    *LL_misses += accum_LL_misses;
}
