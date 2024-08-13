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

static cmeta_t     meta[5];
static uint        meta_count;
static uint        enabled_count;
static double      line_size;
static uint        started;

void set_cache(ullong event, uint type, uint offset, uint used_in_bw, cchar *desc)
{
    meta[meta_count].used_in_bw = used_in_bw;
    meta[meta_count].offset     = offset;
    meta[meta_count].enabled    = 1;
    meta[meta_count].desc       = desc;
    // Trying to open an event ongrpouped
    if (state_fail(perf_open(&meta[meta_count].perf, NULL, 0, type, event))) {
        debug("perf_open for event %s returned: %s", desc, state_msg);
        meta[meta_count].enabled = 0;
    } else {
        debug("perf_open for event %s: OK", desc);
        enabled_count++;
    }
    meta_count++;
}

int OFFSET(int level)
{
    #define LBW INT_MAX-0
    // Based on cache_t structure:
    if (level == LBW) return  0;
    if (level ==   1) return  1;
    if (level ==   2) return  2;
    if (level ==   3) return  3;
    if (level ==   4) return -1;
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
    // Saving the last cache level
    line_size = (double) tp->cache_line_size;
    //
    if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_SAPPHIRE_RAPIDS) {
        set_cache(L1D_LD  , PERF_TYPE_HW_CACHE, OFFSET(001), 0, "PERF_L1D_MISS_LD");
        set_cache(0x3f24  , PERF_TYPE_RAW     , OFFSET(002), 0, "L2_RQSTS.MISS"   );
        set_cache(0x1f25  , PERF_TYPE_RAW     , OFFSET(LBW), 0, "L2_LINES_IN.ALL" );
        set_cache(0x0226  , PERF_TYPE_RAW     , OFFSET(LBW), 0, "L2_LINES_OUT.NON_SILENT");
        set_cache(0x0126  , PERF_TYPE_RAW     , OFFSET(LBW), 0, "L2_LINES_OUT.SILENT");
    } else if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_HASWELL_X) {
        set_cache(L1D_LD  , PERF_TYPE_HW_CACHE, OFFSET(001), 0, "PERF_L1D_MISS_LD");
        set_cache(0x3f24  , PERF_TYPE_RAW     , OFFSET(002), 0, "L2_RQSTS.MISS"   ); // Icelake, Skylake, Broadwell, Haswell
        set_cache(0x40f0  , PERF_TYPE_RAW     , OFFSET(LBW), 0, "L2_TRANS.L2_WB"  ); // Icelake, Skylake, Broadwell, Haswell
        set_cache(0x1ff1  , PERF_TYPE_RAW     , OFFSET(LBW), 0, "L2_LINES_IN.ALL" ); // Icelake, Skylake, Broadwell, Haswell
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
    if (!enabled_count) {
        return;
    }
    apis_put(ops->get_info,     cache_perf_get_info);
    apis_put(ops->init,         cache_perf_init);
    apis_put(ops->dispose,      cache_perf_dispose);
    apis_put(ops->read,         cache_perf_read);
    apis_put(ops->data_diff,    cache_perf_data_diff);
    apis_put(ops->internals_tostr,cache_perf_internals_tostr);
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
    int i;
	if (started) {
        return EAR_SUCCESS;
    }
    for (i = 0; i < meta_count; ++i) {
        if (meta[i].enabled) {
            perf_start(&meta[i].perf);
        }
    }
    ++started;
	return EAR_SUCCESS;
}

state_t cache_perf_dispose()
{
  if (started){
    for (int i = 0; i < meta_count; ++i) {
        if (meta[i].enabled) {
            perf_close(&meta[i].perf);
        }
    }
  }
  started = 0;
	meta_count = 0;
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
    timestamp_get(&ca->time);
    for (i = 0; i < meta_count; ++i) {
        if (meta[i].enabled) {
            if (state_fail(s = perf_read(&meta[i].perf, (llong *) &p[i]))) {
                return s;
            }
        }
    }
    #if SHOW_DEBUGS
	for (i = 0; i < meta_count; ++i) {
        debug("values[%d]: %llu", i, p[i]);
    }
    #endif
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
    for (i = 0; i < meta_count; ++i) {
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

void cache_perf_internals_tostr(char *buffer, int length)
{
    cchar *state[] = { "failed", "loaded" };
    cchar *bwtoo[] = { "", "(also in LBW)" };
    int i, c;
    for (i = c = 0; i < meta_count; ++i) {
        c += sprintf(&buffer[c], "%s: %s (event %s) %s\n", OFFSTR(meta[i].offset),
             state[meta[i].enabled], meta[i].desc, bwtoo[meta[i].used_in_bw]);
    }
}
