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

#include <pthread.h>
#include <metrics/cache/cache.h>
#include <metrics/cache/archs/perf.h>
#include <metrics/cache/archs/dummy.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static cache_ops_t ops;
static uint init;
static uint api;

state_t cache_load(topology_t *tp, int eard)
{
	while (pthread_mutex_trylock(&lock));
	// Already initialized
	if (api != API_NONE) {
		return EAR_SUCCESS;
	}
	api = API_DUMMY;
	if (state_ok(cache_perf_load(tp, &ops))) {
		api = API_PERF;
	}
	cache_dummy_load(tp, &ops);
	init = 1;
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t cache_get_api(uint *api_in)
{
	*api_in = api;
	return EAR_SUCCESS;
}

state_t cache_init(ctx_t *c)
{
	while (pthread_mutex_trylock(&lock));
	state_t s = ops.init(c);
	pthread_mutex_unlock(&lock);
	return s;
}

state_t cache_dispose(ctx_t *c)
{
	return ops.dispose(c);
}

state_t cache_read(ctx_t *c, cache_t *ca)
{
	return ops.read(c, ca);
}

// Helpers
state_t cache_read_diff(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
	state_t s;
	if (state_fail(s = ops.read(c, ca2))) {
		return s;
	}
	return ops.data_diff(ca2, ca1, caD, gbs);
}

state_t cache_read_copy(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
	state_t s;
	if (state_fail(s = cache_read_diff(c, ca2, ca1, caD, gbs))) {
		return s;
	}
	return ops.data_copy(ca1, ca2);
}

state_t cache_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
	return ops.data_diff(ca2, ca1, caD, gbs);
}

state_t cache_data_copy(cache_t *dst, cache_t *src)
{
	return ops.data_copy(dst, src);
}

state_t cache_data_print(cache_t *caD, double gbs, int fd)
{
	return ops.data_print(caD, gbs, fd);
}

state_t cache_data_tostr(cache_t *caD, double gbs, char *buffer, size_t length)
{
	return ops.data_tostr(caD, gbs, buffer, length);
}
