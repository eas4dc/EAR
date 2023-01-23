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
#include <metrics/cache/archs/perf.h>

#define HW_CACHE_L1D  PERF_COUNT_HW_CACHE_L1D
#define HW_CACHE_LL   PERF_COUNT_HW_CACHE_LL
#define HW_OP_READ    PERF_COUNT_HW_CACHE_OP_READ
#define HW_OP_WRITE   PERF_COUNT_HW_CACHE_OP_WRITE
#define HW_RES_MISS   PERF_COUNT_HW_CACHE_RESULT_MISS

// It seems that you can't mix OP_READ and OP_WRITE.
static ullong hwell_l1_ev  = HW_CACHE_L1D  | (HW_OP_READ  << 8) | (HW_RES_MISS << 16);
static ullong hwell_l2_ev  = 0x2724;
static ullong hwell_l3r_ev = HW_CACHE_LL  | (HW_OP_READ  << 8) | (HW_RES_MISS << 16);
static ullong hwell_l3w_ev = HW_CACHE_LL  | (HW_OP_WRITE << 8) | (HW_RES_MISS << 16);
static uint   hwell_l1_tp  = PERF_TYPE_HW_CACHE;
static uint   hwell_l2_tp  = PERF_TYPE_RAW;
static uint   hwell_l3_tp  = PERF_TYPE_HW_CACHE;
static ullong zen_l1_ev    = HW_CACHE_L1D | (HW_OP_READ << 8) | (HW_RES_MISS << 16);
static ullong zen_l3r_ev   = HW_CACHE_LL  | (HW_OP_READ << 8) | (HW_RES_MISS << 16);
static ullong zen_l3w_ev   = HW_CACHE_LL  | (HW_OP_READ << 8) | (HW_RES_MISS << 16);
static uint   zen_l1_tp    = PERF_TYPE_HW_CACHE;
static uint   zen_l3_tp    = PERF_TYPE_HW_CACHE;
//
static perf_t perfs[4];
static uint levels[4];
static uint counter = 0;
static uint started;

void set_cache(ullong event, uint type, uint level)
{
	state_t s;

	if (state_fail(s = perf_open(&perfs[counter], &perfs[0], 0, type, event))) {
		debug("perf_open for level %d returned %d (%s)", level, s, state_msg);
		return;
	}
	levels[counter] = level;
	counter++;
}

state_t cache_perf_load(topology_t *tp, cache_ops_t *ops)
{
	if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) {
		set_cache(hwell_l1_ev, hwell_l1_tp, 1);
		set_cache(hwell_l2_ev, hwell_l2_tp, 2);
		set_cache(hwell_l3r_ev, hwell_l3_tp, 3);
        set_cache(hwell_l3w_ev, hwell_l3_tp, 4);
	}
	else if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
		set_cache(zen_l1_ev, zen_l1_tp, 1);
		set_cache(zen_l3r_ev, zen_l3_tp, 3);
        set_cache(zen_l3w_ev, zen_l3_tp, 4);
	}
	else {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
    if (!counter) {
        return_msg(EAR_ERROR, Generr.api_incompatible);
    }
	replace_ops(ops->init,    cache_perf_init);
	replace_ops(ops->dispose, cache_perf_dispose);
	replace_ops(ops->read,    cache_perf_read);
	return EAR_SUCCESS;
}

state_t cache_perf_init(ctx_t *c)
{
	if (!started) {
		perf_start(&perfs[0]);
		++started;
	}
	return EAR_SUCCESS;
}

state_t cache_perf_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t cache_perf_read(ctx_t *c, cache_t *ca)
{
	llong values[4];
	state_t s;
	int i;

	memset(ca, 0, sizeof(cache_t));
	values[0] = 0LLU;
	values[1] = 0LLU;
	values[2] = 0LLU;
    values[3] = 0LLU;
	if (state_fail(s = perf_read(&perfs[0], values))) {
		return s;
	}
	timestamp_get(&ca->time);
	for (i = 0; i < counter; ++i) {
		if (levels[i] == 1) {
			ca->l1d_misses = values[i];
		} else if (levels[i] == 2) {
			ca->l2_misses = values[i];
		} else if (levels[i] == 3) {
			ca->l3r_misses = values[i];
		} else if (levels[i] == 4) {
            ca->l3w_misses = values[i];
        }
	}
	debug("values[0]: %lld", values[0]);
	debug("values[1]: %lld", values[1]);
    debug("values[2]: %lld", values[2]);
    debug("values[3]: %lld", values[3]);
	debug("total L1D misses: %lld", ca->l1d_misses);
	debug("total L2 misses : %lld", ca->l2_misses);
    debug("total L3R misses : %lld", ca->l3r_misses);
    debug("total L3W misses : %lld", ca->l3w_misses);

	return EAR_SUCCESS;
}
