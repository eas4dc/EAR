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

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/bandwidth/archs/dummy.h>
#include <daemon/local_api/eard_api_rpc.h>

static double line_size;
static uint devs_count;

state_t bwidth_dummy_load(topology_t *tp, bwidth_ops_t *ops)
{
	replace_ops(ops->init,            bwidth_dummy_init);
	replace_ops(ops->dispose,         bwidth_dummy_dispose);
	replace_ops(ops->count_devices,   bwidth_dummy_count_devices);
	replace_ops(ops->get_granularity, bwidth_dummy_get_granularity);
	replace_ops(ops->read,            bwidth_dummy_read);
    replace_ops(ops->data_diff,       bwidth_dummy_data_diff);
    replace_ops(ops->data_accum,      bwidth_dummy_data_accum);
	replace_ops(ops->data_alloc,      bwidth_dummy_data_alloc);
	replace_ops(ops->data_free,       bwidth_dummy_data_free);
	replace_ops(ops->data_copy,       bwidth_dummy_data_copy);
	replace_ops(ops->data_print,      bwidth_dummy_data_print);
	replace_ops(ops->data_tostr,      bwidth_dummy_data_tostr);
	line_size = (double) tp->cache_line_size;
	return EAR_SUCCESS;
}

state_t bwidth_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t bwidth_dummy_init_static(ctx_t *c, bwidth_ops_t *ops)
{
	// Ask for devices
	return ops->count_devices(c, &devs_count);
}

state_t bwidth_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t bwidth_dummy_count_devices(ctx_t *c, uint *devs_count_in)
{
	*devs_count_in = 1;
	return EAR_SUCCESS;
}

state_t bwidth_dummy_get_granularity(ctx_t *c, uint *granularity)
{
	*granularity = GRANULARITY_NODE;
	return EAR_SUCCESS;
}

state_t bwidth_dummy_read(ctx_t *c, bwidth_t *b)
{
	memset(b, 0, (devs_count)*sizeof(bwidth_t));
	return EAR_SUCCESS;
}

state_t bwidth_dummy_data_diff(bwidth_t *b2, bwidth_t *b1, bwidth_t *bD, ullong *cas, double *gbs)
{
	ullong tcas = 0LLU;
    ullong diff = 0LLU;
	ullong time = 0LLU;
	double secs = 0.0;
    int it = devs_count-1;
	int i;
    
    if (cas != NULL) {
        *cas = 0LLU;
    }
    if (gbs != NULL) {
        *gbs = 0.0;
    }
	// Ms to seconds (for decimals)
	time = timestamp_diff(&b2[it].time, &b1[it].time, TIME_MSECS);
	// If no time, no metrics
	if (time == 0LLU) {
		return EAR_SUCCESS;
	}
	secs = (double) time;
	secs = secs / 1000.0;

    if (bD != NULL) {
        bD[it].secs = secs;
    }
	//
	for (i = 0; i < devs_count-1; ++i) {
        diff  = overflow_mixed_u64(b2[i].cas, b1[i].cas, MAXBITS48, 4);
		tcas += diff;
        if (bD != NULL) {
            bD[i].cas = diff;
        }
		debug("IMC%d/%u: %llu - %llu = %llu", i, devs_count-2, b2[i].cas, b1[i].cas, diff);
	}
    debug("CAS: %llu in %0.2lf secs", tcas, secs);
    if (cas != NULL) {
        *cas = tcas;
    }
    if (gbs != NULL) {
        *gbs = bwidth_dummy_help_castogbs(tcas, secs);
    }
	return EAR_SUCCESS;
}

state_t bwidth_dummy_data_accum(bwidth_t *bA, bwidth_t *bD, ullong *cas, double *gbs)
{
    ullong tcas = 0LLU;
    int it = devs_count-1;
    int i;

    for (i = 0; i < devs_count-1; ++i) {
        if (bD != NULL) {
            bA[i].cas += bD[i].cas;
        }
        tcas += bA[i].cas;
    }
    if (bD != NULL) {
        bA[it].secs += bD[it].secs;
    }
    if (cas != NULL) {
        *cas = tcas;
    }
    if (gbs != NULL) {
        *gbs = bwidth_dummy_help_castogbs(tcas, bA[it].secs);
    }
    debug("CAS: %llu in %0.2lf secs", tcas, bA[it].secs);
    return EAR_SUCCESS;
}

state_t bwidth_dummy_data_alloc(bwidth_t **b)
{
	*b = calloc(devs_count+1, sizeof(bwidth_t));
	return EAR_SUCCESS;
}

state_t bwidth_dummy_data_free(bwidth_t **b)
{
	free(*b);
	*b = NULL;
	return EAR_SUCCESS;
}

state_t bwidth_dummy_data_copy(bwidth_t *dst, bwidth_t *src)
{
	memcpy(dst, src, (devs_count+1)*sizeof(bwidth_t));
	return EAR_SUCCESS;
}

state_t bwidth_dummy_data_print(ullong cas, double gbs, int fd)
{
	char buffer[SZ_BUFFER];
	state_t s;
	if (state_fail(s = bwidth_dummy_data_tostr(cas, gbs, buffer, SZ_BUFFER))) {
		return s;
	}
	dprintf(fd, "%s", buffer);
	return EAR_SUCCESS;
}

state_t bwidth_dummy_data_tostr(ullong cas, double gbs, char *buffer, size_t length)
{
#if 1
	snprintf(buffer, length, "%0.2lf (GB/s, %llu CAS)\n", gbs, cas);
#else
	size_t accum = 0;
	size_t added = 0;
	uint i;

	for (i = 0; i < devs_count && length > 0; ++i) {
		added = snprintf(&buffer[accum], length,
			"IMC%u: %llu\n", i, b[i].cas);
		length = length - added;
		accum  = accum  + added;
	}
	snprintf(&buffer[accum], length, "Total: %lf GB/s\n", gbs);
#endif
	return EAR_SUCCESS;
}

double bwidth_dummy_help_castogbs(ullong cas, double secs)
{
    double tgbs = (double) cas;
    tgbs = (tgbs / secs);
    tgbs = (tgbs * line_size);
    tgbs = (tgbs / B_TO_GB);
    return tgbs;
}

double bwidth_dummy_help_castotpi(ullong cas, ullong instructions)
{
    return (((double) cas) * line_size) / ((double) instructions);
}
