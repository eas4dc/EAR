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
#include <metrics/common/perf.h>
#include <metrics/flops/archs/perf.h>

// 0       1       2      3      4      5      6      7      8
// CPU-SP, CPU-DP, 128SP, 128DP, 256SP, 256DP, 512HP, 512SP, 512DP
static ullong arm8a[6]     = { 0x80c3, 0x80c5, 0x80c7, 0x80c2, 0x80c4, 0x80c6 }; // FIXED 16-32-64, SVE-16-32-64
static ullong hwell_evs[8] = { 0x02c7, 0x01c7, 0x08c7, 0x04c7, 0x20c7, 0x10c7, 0x80c7, 0x40c7 };
static ullong zen_evs[1]   = { 0xff03 }; // FpRetSseAvxOps (Old event: 0x04cb)
static ullong weights[8];
static uint   levels[8];
static double mults[8];
static perf_t perfs[8];
static uint   counter;
static uint   started;

void set_flops(ullong event, uint type, uint level, ullong weight, double multiplier)
{
    state_t s;

    if (state_fail(s = perf_open(&perfs[counter], NULL, 0, type, (ulong) event))) {
        debug("perf_open returned %d (%s)", s, state_msg);
        return;
    }
    mults[counter]  = multiplier;
    levels[counter] = level;
    weights[level]  = weight;
    counter++;
}

void flops_perf_load(topology_t *tp, flops_ops_t *ops)
{
	if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) {
		set_flops(hwell_evs[0], PERF_TYPE_RAW, 0,  1, 1.0); // SP
		set_flops(hwell_evs[1], PERF_TYPE_RAW, 1,  1, 1.0); // DP
		set_flops(hwell_evs[2], PERF_TYPE_RAW, 2,  4, 1.0); // 128SP
		set_flops(hwell_evs[3], PERF_TYPE_RAW, 3,  2, 1.0); // 128DP
		set_flops(hwell_evs[4], PERF_TYPE_RAW, 4,  8, 1.0); // 256SP
		set_flops(hwell_evs[5], PERF_TYPE_RAW, 5,  4, 1.0); // 256DP
		set_flops(hwell_evs[6], PERF_TYPE_RAW, 6, 16, 1.0); // 512SP
		set_flops(hwell_evs[7], PERF_TYPE_RAW, 7,  8, 1.0); // 512DP
	}
	else if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
        // This is not precise, but can tell something about AVX computing
		set_flops(zen_evs[0], PERF_TYPE_RAW, 4, 8, 1.0); // We have to check this again
	}
    else if (tp->vendor == VENDOR_ARM) { // && ARMv8a or greater) {
        // The NEON (128 bit) event has to be tested, so we are just going to SVE.
        set_flops(arm8a[0], PERF_TYPE_RAW, 0, 1, 1.0); // FIXED values works for float, double, but
        set_flops(arm8a[1], PERF_TYPE_RAW, 0, 1, 1.0); // also for SIMD NEON. Which is a pity because
        set_flops(arm8a[2], PERF_TYPE_RAW, 1, 1, 1.0); // we wanted to get fine grained results.
        set_flops(arm8a[3], PERF_TYPE_RAW, 6, 1, ((double) tp->sve_bits) / 128.0); // Its is divided to
        set_flops(arm8a[4], PERF_TYPE_RAW, 6, 1, ((double) tp->sve_bits) / 128.0); // match with the tests
        set_flops(arm8a[5], PERF_TYPE_RAW, 7, 1, ((double) tp->sve_bits) / 128.0); // iterations.
    } else {
		debug(Generr.api_incompatible);
        return;
	}
    if (!counter) {
        return;
    }
    apis_put(ops->get_api,         flops_perf_get_api);
    apis_put(ops->get_granularity, flops_perf_get_granularity);
    apis_put(ops->get_weights,     flops_perf_get_weights);
    apis_put(ops->init,            flops_perf_init);
    apis_put(ops->dispose,         flops_perf_dispose);
    apis_put(ops->read,            flops_perf_read);
}

void flops_perf_get_api(uint *api)
{
    *api = API_PERF;
}

void flops_perf_get_granularity(uint *granularity)
{
    *granularity = GRANULARITY_PROCESS;
}

void flops_perf_get_weights(ullong **weights_in)
{
    *weights_in = weights;
}

state_t flops_perf_init(ctx_t *c)
{
	uint i;
	if (!started) {
		for (i = 0; i < counter; ++i) {
			perf_start(&perfs[i]);
		}
		++started;
	}
	return EAR_SUCCESS;
}

state_t flops_perf_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t flops_perf_read(ctx_t *c, flops_t *fl)
{
    ullong *p = (ullong *) fl;
    double dvalue;
    ullong value;
    int i;

    memset(fl, 0, sizeof(flops_t));
    timestamp_get(&fl->time);
    for (i = 0; i < counter; ++i) {
        value = 0LU;
        perf_read(&perfs[i], (long long *) &value);
        debug("values[%d]: %lu", i, value);
        dvalue = (double) value;
        p[levels[i]] += (ullong) (dvalue * mults[i]);
    }
    return EAR_SUCCESS;
}
