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
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <common/system/time.h>
#include <common/output/debug.h>
#include <metrics/flops/flops.h>
#include <metrics/flops/archs/perf.h>
#include <metrics/flops/archs/dummy.h>

// 0       1       2      3      4      5      6      7      8
// CPU-SP, CPU-DP, 128SP, 128DP, 256SP, 256DP, 512HP, 512SP, 512DP
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static flops_ops_t ops;
static uint load;

void flops_load(topology_t *tp, int force_api)
{
	while (pthread_mutex_trylock(&lock));
    if (load) {
        goto out;
    }
    if (API_IS(force_api, API_DUMMY)) {
        goto dummy;
    }
    flops_perf_load(tp, &ops);
dummy:
    flops_dummy_load(tp, &ops);
    load = 1;
out:
	pthread_mutex_unlock(&lock);
}

void flops_get_info(apinfo_t *info)
{
    info->layer       = "FLOPS";
    info->api         = API_NONE;
    info->devs_count  = 0;
    info->scope       = SCOPE_PROCESS;
    info->granularity = GRANULARITY_PROCESS;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t flops_init(ctx_t *c)
{
	while (pthread_mutex_trylock(&lock));
	state_t s = ops.init();
	pthread_mutex_unlock(&lock);
	return s;
}

state_t flops_dispose(ctx_t *c)
{
    load = 0;
	return ops.dispose();
}

state_t flops_read(ctx_t *c, flops_t *fl)
{
	return ops.read(fl);
}

// Helpers
state_t flops_read_diff(ctx_t *c, flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs)
{
	state_t s;
	if (state_ok(s = flops_read(c, fl2))) {
        flops_data_diff(fl2, fl1, flD, gfs);
	}
    return s;
}

state_t flops_read_copy(ctx_t *c, flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs)
{
	state_t s;
	if (state_ok(s = flops_read_diff(c, fl2, fl1, flD, gfs))) {
	    flops_data_copy(fl1, fl2);
	}
    return s;
}

void flops_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs)
{
    return ops.data_diff(fl2, fl1, flD, gfs);
}

void flops_data_copy(flops_t *dst, flops_t *src)
{
    memcpy(dst, src, sizeof(flops_t));
}

void flops_data_print(flops_t *flD, double gfs, int fd)
{
    char buffer[SZ_BUFFER];
    flops_data_tostr(flD, gfs, buffer, SZ_BUFFER);
    dprintf(fd, "%s", buffer);
}

void flops_data_tostr(flops_t *flD, double gfs, char *buffer, size_t length)
{
    int added = 0;
    added += sprintf(&buffer[added], "Total x86 insts f/d: %llu %llu\n", flD->f64 , flD->d64 );
    added += sprintf(&buffer[added], "Total 128 insts f/d: %llu %llu\n", flD->f128, flD->d128);
    added += sprintf(&buffer[added], "Total 256 insts f/d: %llu %llu\n", flD->f256, flD->d256);
    added += sprintf(&buffer[added], "Total 512 insts f/d: %llu %llu " , flD->f512, flD->d512);
    added += sprintf(&buffer[added], "(%0.2lf GF/s)\n", gfs);
}

void flops_data_accum(flops_t *flA, flops_t *flD, double *gfs)
{
    // flAccum
    if (flD != NULL) {
        // TODO: overflow control
        flA->f64    += flD->f64;
        flA->d64    += flD->d64;
        flA->f128   += flD->f128;
        flA->d128   += flD->d128;
        flA->f256   += flD->f256;
        flA->d256   += flD->d256;
        flA->f512   += flD->f512;
        flA->d512   += flD->d512;
        flA->secs   += flD->secs;
        flA->gflops += flD->gflops;
    }
    if (gfs != NULL) {
        *gfs = flA->gflops;
    }
}

void flops_internals_print(int fd)
{
    char buffer[1024];
    flops_internals_tostr(buffer, 1024);
    dprintf(fd, "%s", buffer);
}

void flops_internals_tostr(char *buffer, int length)
{
    ops.internals_tostr(buffer, length);
}

ullong *flops_help_toold(flops_t *flD, ullong *flops)
{
    flops[0] = flD->f64;
    flops[4] = flD->d64;
    flops[1] = flD->f128;
    flops[5] = flD->d128;
    flops[2] = flD->f256;
    flops[6] = flD->d256;
    flops[3] = flD->f512;
    flops[7] = flD->d512;
    debug("flops[0]: %llu", flops[0]);
    debug("flops[1]: %llu", flops[1]);
    debug("flops[2]: %llu", flops[2]);
    debug("flops[3]: %llu", flops[3]);
    debug("flops[4]: %llu", flops[4]);
    debug("flops[5]: %llu", flops[5]);
    debug("flops[6]: %llu", flops[6]);
    debug("flops[7]: %llu", flops[7]);
    return flops;
}
