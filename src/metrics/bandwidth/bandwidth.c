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

#include <pthread.h>
#include <common/output/debug.h>
#include <common/utils/overhead.h>
#include <metrics/bandwidth/bandwidth.h>
#include <metrics/bandwidth/archs/eard.h>
#include <metrics/bandwidth/archs/dummy.h>
#include <metrics/bandwidth/archs/bypass.h>
#include <metrics/bandwidth/archs/likwid.h>
#include <metrics/bandwidth/archs/amd17.h>
#include <metrics/bandwidth/archs/intel63.h>
#include <metrics/bandwidth/archs/intel106.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static bwidth_ops_t    ops;
static uint            api;
static uint            oid;

state_t bwidth_load(topology_t *tp, int eard)
{
	while (pthread_mutex_trylock(&lock));
	// Already initialized
	if (api != API_NONE) {
		pthread_mutex_unlock(&lock);
		return EAR_SUCCESS;
	}
  #ifndef FAKE_ERROR_USE_DUMMY
	// Dummy by default
	api = API_DUMMY;
	if (state_ok(bwidth_eard_load(tp, &ops, eard))) {
		debug("Loaded EARD");
		api = API_EARD;
	}
	if (state_ok(bwidth_intel63_load(tp, &ops))) {
		api = API_INTEL63;
		debug("Loaded INTEL63");
	}
	if (state_ok(bwidth_amd17_load(tp, &ops))) {
		api = API_AMD17;
		debug("Loaded AMD17");
	}
    // EAR memory bandwith plugin for Icelake using Likwid
	if (state_ok(bwidth_likwid_load(tp, &ops))) {
		api = API_LIKWID;
		debug("Loaded LIKWID");
	}
    // EAR memory bandwith plugin for Icelake
	if (state_ok(bwidth_intel106_load(tp, &ops))) {
		api = API_INTEL106;
		debug("Loaded INTEL106");
	}
    #if 0
	if (state_ok(bwidth_bypass_load(tp, &ops))) {
		api = API_BYPASS;
		debug("Loaded BYPASS");
	}
    #endif
    #endif
	bwidth_dummy_load(tp, &ops);
    // Overhead control
    overhead_suscribe("metrics/bandwidth", &oid);
    // Leaving
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t bwidth_get_api(uint *api_in)
{
	*api_in = api;
	return EAR_SUCCESS;
}

state_t bwidth_init(ctx_t *c)
{
	while (pthread_mutex_trylock(&lock));
	state_t s = ops.init(c);
	if (state_ok(bwidth_dummy_init_static(c, &ops))) {
		// Debug
	}
	pthread_mutex_unlock(&lock);
	return s;
}

state_t bwidth_dispose(ctx_t *c)
{
	return ops.dispose(c);
}

state_t bwidth_count_devices(ctx_t *c, uint *dev_count)
{
	return ops.count_devices(c, dev_count);
}

state_t bwidth_get_granularity(ctx_t *c, uint *granularity)
{
	return ops.get_granularity(c, granularity);
}

state_t bwidth_read(ctx_t *c, bwidth_t *b)
{
    overhead_start(oid);
    state_t s = ops.read(c, b);
    overhead_stop(oid);
    return s;
}

state_t bwidth_read_diff(ctx_t *c, bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs)
{
	state_t s;
	if (state_fail(s = bwidth_read(c, b2))) {
		return s;
	}
	return ops.data_diff(b2, b1, bD, cas, gbs);
}

state_t bwidth_read_copy(ctx_t *c, bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs)
{
	state_t s;
	if (state_fail(s = bwidth_read_diff(c, b2, b1, bD, cas, gbs))) {
		return s;
	}
	return ops.data_copy(b1, b2);
}

state_t bwidth_data_diff(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs)
{
	return ops.data_diff(b2, b1, bD, cas, gbs);
}

state_t bwidth_data_accum(bwidth_t *bA, bwidth_t *bD, ullong *cas, double *gbs)
{
	return ops.data_accum(bA, bD, cas, gbs);
}

state_t bwidth_data_alloc(bwidth_t **b)
{
	return ops.data_alloc(b);
}

state_t bwidth_data_free(bwidth_t **b)
{
	return ops.data_free(b);
}

state_t bwidth_data_copy(bwidth_t *dst, bwidth_t *src)
{
	return ops.data_copy(dst, src);
}

state_t bwidth_data_print(ullong cas, double gbs, int fd)
{
	return ops.data_print(cas, gbs, fd);
}

state_t bwidth_data_tostr(ullong cas, double gbs, char *buffer, size_t length)
{
	return ops.data_tostr(cas, gbs, buffer, length);
}

double bwidth_help_castogbs(ullong cas, double secs)
{
    return bwidth_dummy_help_castogbs(cas, secs);
}

double bwidth_help_castotpi(ullong cas, ullong instructions)
{
    return bwidth_dummy_help_castotpi(cas, instructions);
}
