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

#include <pthread.h>
// #define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <metrics/flops/flops.h>
#include <metrics/flops/archs/perf.h>
#include <metrics/flops/archs/dummy.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static flops_ops_t ops;
static uint init;
static uint api;

state_t flops_load(topology_t *tp, int eard)
{
	while (pthread_mutex_trylock(&lock));
	// Already initialized
	if (api != API_NONE) {
		return EAR_SUCCESS;
	}
	api = API_DUMMY;
	if (state_ok(flops_perf_load(tp, &ops))) {
		api = API_PERF;
	}
	flops_dummy_load(tp, &ops);
	init = 1;
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t flops_get_api(uint *api_in)
{
	*api_in = api;
	return EAR_SUCCESS;
}

state_t flops_init(ctx_t *c)
{
	while (pthread_mutex_trylock(&lock));
	state_t s = ops.init(c);
	pthread_mutex_unlock(&lock);
	return s;
}

state_t flops_dispose(ctx_t *c)
{
	return ops.dispose(c);
}

state_t flops_read(ctx_t *c, flops_t *fl)
{
	return ops.read(c, fl);
}

// Helpers
state_t flops_read_diff(ctx_t *c, flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs)
{
	state_t s;
	if (state_fail(s = ops.read(c, fl2))) {
		return s;
	}
	return ops.data_diff(fl2, fl1, flD, gfs);
}

state_t flops_read_copy(ctx_t *c, flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs)
{
	state_t s;
	if (state_fail(s = flops_read_diff(c, fl2, fl1, flD, gfs))) {
		return s;
	}
	return ops.data_copy(fl2, fl1);
}

state_t flops_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs)
{
	return ops.data_diff(fl2, fl1, flD, gfs);
}

state_t flops_data_accum(flops_t *flA, flops_t *flD, double *gfs)
{
    return ops.data_accum(flA, flD, gfs);
}

state_t flops_data_copy(flops_t *dst, flops_t *src)
{
	return ops.data_copy(dst, src);
}

state_t flops_data_print(flops_t *flD, double gfs, int fd)
{
	return ops.data_print(flD, gfs, fd);
}

state_t flops_data_tostr(flops_t *flD, double gfs, char *buffer, size_t length)
{
	return ops.data_tostr(flD, gfs, buffer, length);
}

ullong *flops_help_toold(flops_t *flD, ullong *old_flops)
{
    return flops_dummy_help_toold(flD, old_flops);
}