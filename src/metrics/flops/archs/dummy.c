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

#include <common/system/time.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/common/perf.h>
#include <metrics/flops/archs/dummy.h>

static ullong weights[8] = { 1, 1, 4, 2, 8, 4, 16, 8 };

state_t flops_dummy_load(topology_t *tp, flops_ops_t *ops)
{
	replace_ops(ops->init,       flops_dummy_init);
	replace_ops(ops->dispose,    flops_dummy_dispose);
	replace_ops(ops->read,       flops_dummy_read);
    replace_ops(ops->data_diff,  flops_dummy_data_diff);
    replace_ops(ops->data_accum, flops_dummy_data_accum);
	replace_ops(ops->data_copy,  flops_dummy_data_copy);
	replace_ops(ops->data_print, flops_dummy_data_print);
	replace_ops(ops->data_tostr, flops_dummy_data_tostr);
	return EAR_SUCCESS;
}

state_t flops_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t flops_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t flops_dummy_read(ctx_t *c, flops_t *fl)
{
	memset(fl, 0, sizeof(flops_t));
	return EAR_SUCCESS;
}

// Helpers
static double compute_gflops(flops_t *fl)
{
	double aux = 0.0;

    aux += (double) (fl->f64  * weights[0]);
	aux += (double) (fl->d64  * weights[1]);
	aux += (double) (fl->f128 * weights[2]);
	aux += (double) (fl->d128 * weights[3]);
	aux += (double) (fl->f256 * weights[4]);
	aux += (double) (fl->d256 * weights[5]);
	aux += (double) (fl->f512 * weights[6]);
	aux += (double) (fl->d512 * weights[7]);
	aux = (aux / fl->secs);
	aux = (aux / B_TO_GB);
    debug("Total GFLOPS: %0.2lf", aux);

    return aux;
}

state_t flops_dummy_data_diff(flops_t *fl2, flops_t *fl1, flops_t *flD, double *gfs)
{
	ullong time = 0LLU;
    flops_t flA; // flAux

    if (gfs != NULL) {
        *gfs = 0.0;
    }
    if (flD != NULL) {
        memset(flD, 0, sizeof(flops_t));
    }
	// Ms to seconds (for decimals)
	if ((time = timestamp_diff(&fl2->time, &fl1->time, TIME_MSECS)) == 0LLU) {
		return EAR_SUCCESS;
	}
	flA.secs = (double) time;
    flA.secs = flA.secs / 1000.0;
	// Instruction differences
	flA.f64  = overflow_zeros_u64(fl2->f64 , fl1->f64);
	flA.d64  = overflow_zeros_u64(fl2->d64 , fl1->d64);
	flA.f128 = overflow_zeros_u64(fl2->f128, fl1->f128);
	flA.d128 = overflow_zeros_u64(fl2->d128, fl1->d128);
	flA.f256 = overflow_zeros_u64(fl2->f256, fl1->f256);
	flA.d256 = overflow_zeros_u64(fl2->d256, fl1->d256);
	flA.f512 = overflow_zeros_u64(fl2->f512, fl1->f512);
	flA.d512 = overflow_zeros_u64(fl2->d512, fl1->d512);
	#if 0
	debug("Total f64  insts: %llu", flA.f64);
	debug("Total d64  insts: %llu", flA.d64);
	debug("Total f128 insts: %llu", flA.f128);
	debug("Total d128 insts: %llu", flA.d128);
	debug("Total f256 insts: %llu", flA.f256);
	debug("Total d256 insts: %llu", flA.d256);
	debug("Total f512 insts: %llu", flA.f512);
	debug("Total d512 insts: %llu", flA.d512);
    #endif

    if (flD != NULL) {
        memcpy(flD, &flA, sizeof(flops_t));
    }
    if (gfs != NULL) {
        *gfs = compute_gflops(&flA);
    }
	return EAR_SUCCESS;
}

state_t flops_dummy_data_accum(flops_t *flA, flops_t *flD, double *gfs)
{
    if (flD != NULL) {
    flA->f64  += flD->f64;
    flA->d64  += flD->d64;
    flA->f128 += flD->f128;
    flA->d128 += flD->d128;
    flA->f256 += flD->f256;
    flA->d256 += flD->d256;
    flA->f512 += flD->f512;
    flA->d512 += flD->d512;
    flA->secs += flD->secs;
    }
    if (gfs != NULL) {
        *gfs = compute_gflops(flA);
    }
    return EAR_SUCCESS;
}

state_t flops_dummy_data_copy(flops_t *dst, flops_t *src)
{
	memcpy(dst, src, sizeof(flops_t));
	return EAR_SUCCESS;
}

state_t flops_dummy_data_print(flops_t *flD, double gfs, int fd)
{
	char buffer[SZ_BUFFER];
	if (state_ok(flops_dummy_data_tostr(flD, gfs, buffer, SZ_BUFFER))) {
		dprintf(fd, "%s", buffer);
	}
	return EAR_SUCCESS;
}

state_t flops_dummy_data_tostr(flops_t *flD, double gfs, char *buffer, size_t length)
{
	int added = 0;
	added += sprintf(&buffer[added], "Total  64 insts f-d: %llu %llu\n", flD->f64 , flD->d64);
	added += sprintf(&buffer[added], "Total 128 insts f-d: %llu %llu\n", flD->f128, flD->d128);
	added += sprintf(&buffer[added], "Total 256 insts f-d: %llu %llu\n", flD->f256, flD->d256);
	added += sprintf(&buffer[added], "Total 512 insts f-d: %llu %llu\n", flD->f512, flD->d512);
	added += sprintf(&buffer[added], "Total GFLOPS: %0.2lf\n", gfs);
	return EAR_SUCCESS;
}

ullong *flops_dummy_help_toold(flops_t *flD, ullong *flops)
{
    flops[0] = (flD->f64  * weights[0]);
    flops[4] = (flD->d64  * weights[1]);
    flops[1] = (flD->f128 * weights[2]);
    flops[5] = (flD->d128 * weights[3]);
    flops[2] = (flD->f256 * weights[4]);
    flops[6] = (flD->d256 * weights[5]);
    flops[3] = (flD->f512 * weights[6]);
    flops[7] = (flD->d512 * weights[7]);
    compute_gflops(flD);
    return flops;
}
