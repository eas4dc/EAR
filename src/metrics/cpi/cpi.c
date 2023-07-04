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
#include <metrics/cpi/cpi.h>
#include <metrics/cpi/archs/perf.h>
#include <metrics/cpi/archs/dummy.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static cpi_ops_t ops;
static uint init;
static uint api;

state_t cpi_load(topology_t *tp, int eard)
{
	while (pthread_mutex_trylock(&lock));
	// Already initialized
	if (api != API_NONE) {
		goto leave;
	}
	api = API_DUMMY;
	if (state_ok(cpi_perf_load(tp, &ops))) {
		api = API_PERF;
	}
	cpi_dummy_load(tp, &ops);
	init = 1;
leave:
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t cpi_get_api(uint *api_in)
{
	*api_in = api;
	return EAR_SUCCESS;
}

state_t cpi_init(ctx_t *c)
{
	while (pthread_mutex_trylock(&lock));
	state_t s = ops.init(c);
	pthread_mutex_unlock(&lock);
	return s;
}

state_t cpi_dispose(ctx_t *c)
{
	return ops.dispose(c);
}

state_t cpi_read(ctx_t *c, cpi_t *ci)
{
	return ops.read(c, ci);
}

// Helpers
state_t cpi_read_diff(ctx_t *c, cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi)
{
	state_t s;
	if (state_fail(s = ops.read(c, ci2))) {
		return s;
	}
	return ops.data_diff(ci2, ci1, ciD, cpi);
}

state_t cpi_read_copy(ctx_t *c, cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi)
{
	state_t s;
	if (state_fail(s = cpi_read_diff(c, ci2, ci1, ciD, cpi))) {
		return s;
	}
	return ops.data_copy(ci1, ci2);
}

state_t cpi_data_diff(cpi_t *ci2, cpi_t *ci1, cpi_t *ciD, double *cpi)
{
	return ops.data_diff(ci2, ci1, ciD, cpi);
}

state_t cpi_data_copy(cpi_t *dst, cpi_t *src)
{
	return ops.data_copy(dst, src);
}

state_t cpi_data_print(cpi_t *ciD, double cpi, int fd)
{
	return ops.data_print(ciD, cpi, fd);
}

state_t cpi_data_tostr(cpi_t *ciD, double cpi, char *buffer, size_t length)
{
	return ops.data_tostr(ciD, cpi, buffer, length);
}
