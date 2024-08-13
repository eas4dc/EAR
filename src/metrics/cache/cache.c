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

#include <pthread.h>
#include <common/output/debug.h>
#include <metrics/cache/cache.h>
#include <metrics/cache/archs/perf.h>
#include <metrics/cache/archs/dummy.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static cache_ops_t ops;
static uint load;

void cache_load(topology_t *tp, int force_api)
{
	while (pthread_mutex_trylock(&lock));
    if (load) {
        goto out;
    }
    if (API_IS(force_api, API_DUMMY)) {
        goto dummy;
    }
    cache_perf_load(tp, &ops);
dummy:
    cache_dummy_load(tp, &ops);
    // Getting cache line, is used to get the bandwidth
	load = 1;
out:
	pthread_mutex_unlock(&lock);
	return;
}

void cache_get_info(apinfo_t *info)
{
    info->layer       = "CACHE";
    info->api         = API_NONE;
    info->devs_count  = 0;
    info->scope       = SCOPE_PROCESS;
    info->granularity = GRANULARITY_PROCESS;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t cache_init(ctx_t *c)
{
	while (pthread_mutex_trylock(&lock));
	state_t s = ops.init();
	pthread_mutex_unlock(&lock);
	return s;
}

state_t cache_dispose(ctx_t *c)
{
  load = 0;
	return ops.dispose();
}

state_t cache_read(ctx_t *c, cache_t *ca)
{
	return ops.read(ca);
}

state_t cache_read_diff(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
	state_t s;
	if (state_fail(s = cache_read(c, ca2))) {
		return s;
	}
	cache_data_diff(ca2, ca1, caD, gbs);
    return s;
}

state_t cache_read_copy(ctx_t *c, cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
	state_t s;
	if (state_fail(s = cache_read_diff(c, ca2, ca1, caD, gbs))) {
		return s;
	}
	cache_data_copy(ca1, ca2);
    return s;
}

void cache_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
    return ops.data_diff(ca2, ca1, caD, gbs);
}

void cache_data_copy(cache_t *dst, cache_t *src)
{
    memcpy(dst, src, sizeof(cache_t));
}

void cache_data_print(cache_t *ca, double gbs, int fd)
{
    char buffer[SZ_BUFFER];
    cache_data_tostr(ca, gbs, buffer, SZ_BUFFER);
    dprintf(fd, "%s", buffer);
}

void cache_data_tostr(cache_t *ca, double gbs, char *buffer, size_t length)
{
snprintf(buffer, length,
    "L1D misses: %llu\nL2  misses: %llu\nL3  misses: %llu\nLL  misses: %llu\nLBW misses: %llu (%0.2lf GB/s)\n",
    ca->l1d_misses, ca->l2_misses, ca->l3_misses, ca->ll_misses, ca->lbw_misses, gbs);
}

void cache_internals_print(int fd)
{
    char buffer[1024];
    cache_internals_tostr(buffer, 1024);
    dprintf(fd, "%s", buffer);
}

void cache_internals_tostr(char *buffer, int length)
{
    ops.internals_tostr(buffer, length);
}
