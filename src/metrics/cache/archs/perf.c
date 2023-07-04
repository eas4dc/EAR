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
#include <metrics/cache/archs/perf.h>

#define L1D_LD  PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ  << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define L1D_ST  PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)

// - We count bandwidth as the data loading to LL from upper leverls (read), or
//   stored in upper levels by LL (write). This is because we are interested in
//   the data traffic to DRAM, not in inter cache communications. LL (last level)
//   could be L3 or L2. The main events are LL_MISS and LL_WRITE_BACK.
// - It seems that you can't mix OP_READ and OP_WRITE.
// 
// Skylake:
//   - It has a victim cache, so most of the data avoids the L3, passing directly
//     from L2 to DRAM. (Look LIKWID Github: L2-L3-MEM-traffic-on-Intel-Skylake)
//   - Intel don't offer a a PERF_STORE event for L1D cache.
//

typedef struct cmeta_s {
    perf_t perf;
    int    offset;
    int    used_in_bw;
    int    enabled;
    cchar  *desc;
} cmeta_t;

static topology_t *tp;
static cmeta_t     meta[5];
static uint        counter;
static uint        started;
static uint        first; // First perf which worked
static uint        cache_last_level;
static double      line_size;

void set_cache(ullong event, uint type, uint offset, uint used_in_bw, cchar *desc)
{
    if (state_fail(perf_open(&meta[counter].perf, &meta[first].perf, 0, type, event))) {
        debug("perf_open for level %d returned: %s", offset, state_msg);
        return;
    }
    meta[counter].used_in_bw = used_in_bw;
    meta[counter].offset     = offset;
    meta[counter].enabled    = 1;
    meta[counter].desc       = desc;
    debug("loaded event %s", desc);
    counter++;
}

int OFFSET(int level)
{
    #define LBW INT_MAX-0
    #define LL  INT_MAX-1
    #define LX  INT_MAX-2
    // Based on cache_t structure:
    if (level == LBW) return  0;
    if (level ==   1) return  1;
    if (level ==   2) return  2;
    if (level ==   3) return  3;
    if (level ==   4) return -1;
    if (level ==  LL) return  4;
    if (level ==  LX) return OFFSET(cache_last_level);
    return -1;
}

char *OFFSTR(int offset)
{
    if (offset ==   0) return "LBW";
    if (offset ==   1) return "L1D";
    if (offset ==   2) return "L2 ";
    if (offset ==   3) return "L3 ";
    return "L? ";
}

void cache_perf_load(topology_t *tp, cache_ops_t *ops)
{
    // Saving the last cache level (our LX)
    cache_last_level = tp->cache_last_level;
    line_size = (double) tp->cache_line_size;
    //
    if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) {
        set_cache(L1D_LD  , PERF_TYPE_HW_CACHE, OFFSET(001), 0, "PERF_L1D_MISS_LD");
        set_cache(0x3f24  , PERF_TYPE_RAW     , OFFSET(002), 0, "L2_RQSTS.MISS"   );
        set_cache(0x40f0  , PERF_TYPE_RAW     , OFFSET(LBW), 0, "L2_TRANS.L2_WB"  );
        set_cache(0x1ff1  , PERF_TYPE_RAW     , OFFSET(LBW), 0, "L2_LINES_IN.ALL" );
    } else if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
        set_cache(L1D_LD  , PERF_TYPE_HW_CACHE, OFFSET(001), 0, "PERF_L1D_MISS_LD");
        set_cache(0x430864, PERF_TYPE_RAW     , OFFSET(002), 1, "L2_MISS_FROM_DC" );
        set_cache(0x431f71, PERF_TYPE_RAW     , OFFSET(002), 1, "L2_MISS_FROM_PF1");
        set_cache(0x431f72, PERF_TYPE_RAW     , OFFSET(002), 1, "L2_MISS_FROM_PF2");
    } else if (tp->vendor == VENDOR_ARM) {
        set_cache(L1D_LD  , PERF_TYPE_HW_CACHE, OFFSET(001), 0, "PERF_L1D_MISS_LD");
        set_cache(0x17    , PERF_TYPE_RAW     , OFFSET(002), 1, "L2D_CACHE_REFILL");
        set_cache(0x18    , PERF_TYPE_RAW     , OFFSET(LBW), 0, "L2D_CACHE_WB"    );
    }
    if (!counter) {
        return;
    }
    apis_put(ops->get_info,     cache_perf_get_info);
    apis_put(ops->init,         cache_perf_init);
    apis_put(ops->dispose,      cache_perf_dispose);
    apis_put(ops->read,         cache_perf_read);
    apis_put(ops->data_diff,    cache_perf_data_diff);
    apis_put(ops->details_tostr,cache_perf_details_tostr);
    debug("Loaded PERF")
}

void cache_perf_get_info(apinfo_t *info)
{
    info->api         = API_PERF;
    info->scope       = SCOPE_PROCESS;
    info->granularity = GRANULARITY_PROCESS;
    info->devs_count  = 1;
}

state_t cache_perf_init()
{
	if (!started) {
		perf_start(&meta[first].perf);
		++started;
	}
	return EAR_SUCCESS;
}

state_t cache_perf_dispose()
{
	return EAR_SUCCESS;
}

state_t cache_perf_read(cache_t *ca)
{
    ullong *p = (ullong *) ca;
    state_t s;
    int i;
    // Cleaning
    memset(ca, 0, sizeof(cache_t));
    // Reading
	if (state_fail(s = perf_read(&meta[first].perf, (llong *) p))) {
		return s;
	}
	timestamp_get(&ca->time);
	for (i = 0; i < counter; ++i) {
        debug("values[%d]: %llu", i, p[i]);
    }
    return EAR_SUCCESS;
}

void cache_perf_data_diff(cache_t *ca2, cache_t *ca1, cache_t *caD, double *gbs)
{
    ullong *pD = (ullong *) caD;
    ullong *p2 = (ullong *) ca2;
    ullong *p1 = (ullong *) ca1;
    ullong time = 0LLU;
    double secs = 0.0;
    double cas  = 0.0;
    int i;

    memset(caD, 0, sizeof(cache_t));
    // Ms to seconds (for decimals)
    time = timestamp_diff(&ca2->time, &ca1->time, TIME_MSECS);
    if (time == 0LLU) {
        return;
    }
    secs = (double) time;
    secs = secs / 1000.0;
    //
    for (i = 0; i < counter; ++i) {
        if (!meta[i].enabled) {
            continue;
        }
        pD[meta[i].offset] += overflow_zeros_u64(p2[i], p1[i]);
        // If this data is also valid to calc bandwidth
        if (meta[i].used_in_bw) {
            caD->lbw_misses = pD[meta[i].offset];
        }
    }
    // If LX don't have values, but LL have values
    debug("Total L1D misses: %lld", caD->l1d_misses);
    debug("Total L2  misses: %lld", caD->l2_misses );
    debug("Total L3  misses: %lld", caD->l3_misses );
    debug("Total LL  misses: %lld", caD->ll_misses );
    debug("Total LBW access: %lld", caD->lbw_misses);
    // Converting to bandwidth
    cas  = (double) caD->lbw_misses;
    cas  = cas / secs;
    cas  = cas * ((double) line_size);
    cas  = cas / ((double) 1E9);
    *gbs = cas;
}

void cache_perf_details_tostr(char *buffer, int length)
{
    cchar *state[] = { "failed", "loaded" };
    cchar *bwtoo[] = { "", "(also in LBW)" };
    int i, c;
    for (i = c = 0; i < counter; ++i) {
        c += sprintf(&buffer[c], "%s: %s (event %s) %s\n", OFFSTR(meta[i].offset),
             state[meta[i].enabled], meta[i].desc, bwtoo[meta[i].used_in_bw]);
    }
}