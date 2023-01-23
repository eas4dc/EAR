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
#include <metrics/cache/archs/dummy.h>

static double line_size;

state_t cache_dummy_load(topology_t *tp, cache_ops_t *ops)
{
	replace_ops(ops->init,       cache_dummy_init);
	replace_ops(ops->dispose,    cache_dummy_dispose);
	replace_ops(ops->read,       cache_dummy_read);
	replace_ops(ops->data_diff,  cache_dummy_data_diff);
	replace_ops(ops->data_copy,  cache_dummy_data_copy);
	replace_ops(ops->data_print, cache_dummy_data_print);
	replace_ops(ops->data_tostr, cache_dummy_data_tostr);
	// Getting cache line, is used to get the bandwidth
	line_size = (double) tp->cache_line_size;
	return EAR_SUCCESS;
}

state_t cache_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cache_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cache_dummy_read(ctx_t *c, cache_t *ca)
{
	memset(ca, 0, sizeof(cache_t));
	return EAR_SUCCESS;
}

// Helpers
state_t cache_dummy_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
	ullong time = 0LLU;
	double secs = 0.0;
	double cas  = 0.0;

	memset(caD, 0, sizeof(cache_t));
	// Ms to seconds (for decimals)
	time = timestamp_diff(&ca2->time, &ca1->time, TIME_MSECS);
	if (time == 0LLU) {
		return EAR_SUCCESS;
	}
	secs = (double) time;
	secs = secs / 1000.0;
	//
	caD->l1d_misses = overflow_zeros_u64(ca2->l1d_misses, ca1->l1d_misses);
	caD->l2_misses  = overflow_zeros_u64(ca2->l2_misses,  ca1->l2_misses);
    caD->l3r_misses = overflow_zeros_u64(ca2->l3r_misses, ca1->l3r_misses);
    caD->l3w_misses = overflow_zeros_u64(ca2->l3w_misses, ca1->l3w_misses);
    caD->l3_misses  = caD->l3r_misses + caD->l3w_misses;
    debug("Total L1D misses: %lld", caD->l1d_misses);
    debug("Total L2 misses : %lld", caD->l2_misses);
    debug("Total L3 misses : %lld", caD->l3_misses);
	cas = (double) caD->l3_misses;
	cas = (cas / secs);
	cas = (cas * line_size);
	cas = (cas / B_TO_GB);
	*gbs = cas;

	return EAR_SUCCESS;
}

state_t cache_dummy_data_copy(cache_t *dst, cache_t *src)
{
	memcpy(dst, src, sizeof(cache_t));
	return EAR_SUCCESS;
}

state_t cache_dummy_data_print(cache_t *ca, double gbs, int fd)
{
	char buffer[SZ_BUFFER];
	if (state_ok(cache_dummy_data_tostr(ca, gbs, buffer, SZ_BUFFER))) {
		dprintf(fd, "%s", buffer);
	}
	return EAR_SUCCESS;
}

state_t cache_dummy_data_tostr(cache_t *ca, double gbs, char *buffer, size_t length)
{
	 snprintf(buffer, length,
		"L1D misses: %llu\nL2 misses : %llu\nL3 misses : %llu (%0.2lf GB/s)\n",
		ca->l1d_misses, ca->l2_misses, ca->l3_misses, gbs);
	return EAR_SUCCESS;
}
