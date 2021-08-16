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

#include <common/output/debug.h>
#include <metrics/common/perf.h>
#include <metrics/cache/cpu/perf.h>

static llong values[2];
static perf_t perf_llc;
static perf_t perf_l1d;
static uint initialized;

state_t cache_perf_status(topology_t *tp)
{
	return EAR_SUCCESS;
}

state_t cache_perf_init(ctx_t *c)
{
	debug("function");
	state_t s;
	
	if (initialized) {
		return EAR_SUCCESS;
	}

	// Event creation
	ulong event_l1d = PERF_COUNT_HW_CACHE_L1D;
	ulong event_llc = PERF_COUNT_HW_CACHE_LL;
	// Operation (read)
	event_l1d = event_l1d | (PERF_COUNT_HW_CACHE_OP_READ << 8);
	event_llc = event_llc | (PERF_COUNT_HW_CACHE_OP_READ << 8);
	// Result (miss)
	event_l1d = event_l1d | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
	event_llc = event_llc | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

	//
	if (xtate_fail(s, perf_open(&perf_l1d, &perf_l1d, 0, PERF_TYPE_HW_CACHE, event_l1d))) {
		debug("perf_open returned %d (%s)", s, state_msg);
		return s;
	}
	if (xtate_fail(s, perf_open(&perf_llc, &perf_l1d, 0, PERF_TYPE_HW_CACHE, event_llc))) {
		debug("perf_open returned %d (%s)", s, state_msg);
		perf_close(&perf_l1d);
		return s;
	}

	initialized = 1;
	
	return EAR_SUCCESS;
}

state_t cache_perf_dispose(ctx_t *c)
{
	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	
	cache_perf_stop(c, NULL, NULL);
	perf_close(&perf_llc);
	perf_close(&perf_l1d);

	return EAR_SUCCESS;
}

state_t cache_perf_reset(ctx_t *c)
{
	debug("function");
	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	return perf_reset(&perf_l1d);
}

state_t cache_perf_start(ctx_t *c)
{
	debug("function");
	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	return perf_start(&perf_l1d);
}

state_t cache_perf_stop(ctx_t *c, llong *L1_misses, llong *LL_misses)
{
	debug("function");
	state_t s;

	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	if (xtate_fail(s, perf_stop(&perf_l1d))) {
		return s;
	}

	return cache_perf_read(c, L1_misses, LL_misses);
}

state_t cache_perf_read(ctx_t *c, llong *L1_misses, llong *LL_misses)
{
	state_t s;

	if (L1_misses != NULL) *L1_misses = 0;
	if (LL_misses != NULL) *LL_misses = 0;

	if (!initialized) {
		return_msg(EAR_ERROR, Generr.api_uninitialized);
	}
	if (xtate_fail(s, perf_read(&perf_l1d, values))) {
		return s;
	}

	if (L1_misses != NULL) *L1_misses = values[0];
	if (LL_misses != NULL) *LL_misses = values[1];

	debug("total L1 misses %lld", values[0]);
	debug("total LLC misses %lld", values[1]);

	return EAR_SUCCESS;
}

state_t cache_perf_data_print(ctx_t *c, llong L1_misses, llong LL_misses)
{
	return EAR_SUCCESS;
}
