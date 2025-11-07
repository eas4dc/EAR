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

#include <common/math_operations.h>
#include <common/output/debug.h>
#include <common/system/time.h>
#include <metrics/cache/archs/perf.h>
#include <metrics/common/offsets.h>
#include <metrics/common/perf.h>

#define PERF_TYPE_HW PERF_TYPE_HW_CACHE
#define L1_LDM       PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define L1_LDA       PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)

static perf_t perfs[6];
static uint perfs_count;
static uint started;
static offset_ops_t ofs;

static void perfs_add(ullong event, uint type, ulong offset_result, const char *event_desc)
{
    debug("%s", event_desc);
    offsets_add(&ofs, offset_result, NO_OFFSET, NO_OFFSET, '=');
    if (state_fail(perf_open(&perfs[perfs_count], NULL, 0, type, event))) {
        debug("failed: %s", state_msg);
    }
    strcpy(perfs[perfs_count].event_name, event_desc);
    ++perfs_count;
}

static int perfs_count_opened_fds()
{
    int i, counter;
    for (i = counter = 0; i < perfs_count; ++i) {
        counter += (perfs[i].fd >= 0);
    }
    return counter;
}

#define O2(l, v) (((int) offsetof(cache_t, l)) + ((int) offsetof(cache_level_t, v)))
#define O1(l)    (((int) offsetof(cache_t, l)))

void cache_perf_load(topology_t *tp, cache_ops_t *ops)
{
    if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_SKYLAKE_X) {
        perfs_add(0x01d1, PERF_TYPE_RAW, O2(l1d, hits    ), "MEM_LOAD_RETIRED.L1_HIT       ");
        perfs_add(0x08d1, PERF_TYPE_RAW, O2(l1d, misses  ), "MEM_LOAD_RETIRED.L1_MISS      ");
        perfs_add(0xe724, PERF_TYPE_RAW, O2(l2 , accesses), "L2_RQSTS.ALL_DEMAND_REFERENCES");
        perfs_add(0x2724, PERF_TYPE_RAW, O2(l2 , misses  ), "L2_RQSTS.ALL_DEMAND_MISS      ");
        perfs_add(0x04d1, PERF_TYPE_RAW, O2(l3 , hits    ), "MEM_LOAD_RETIRED.L3_HIT       ");
        perfs_add(0x20d1, PERF_TYPE_RAW, O2(l3 , misses  ), "MEM_LOAD_RETIRED.L3_MISS      ");
        offsets_add(&ofs, O2(l1d, accesses), O2(l1d, hits    ), O2(l1d, misses), '+');
        offsets_add(&ofs, O2(l2 , hits    ), O2(l2 , accesses), O2(l2 , misses), '-');
        offsets_add(&ofs, O1(ll ), O1(l3), 0, '&');
        offsets_add(&ofs, O1(lbw), O1(l2), 0, '&');
    } else if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_BROADWELL_X) {
        perfs_add(L1_LDA, PERF_TYPE_HW , O2(l1d, accesses), "PERF_L1D_LOAD_ACCESS");
        perfs_add(L1_LDM, PERF_TYPE_HW , O2(l1d, misses  ), "PERF_L1D_LOAD_MISS");
        perfs_add(0xe724, PERF_TYPE_RAW, O2(l2 , accesses), "L2_RQSTS.ALL_DEMAND_REFERENCES");
        perfs_add(0x2724, PERF_TYPE_RAW, O2(l2 , misses  ), "L2_RQSTS.ALL_DEMAND_MISS      ");
        offsets_add(&ofs, O2(l1d, hits), O2(l1d, accesses), O2(l1d, misses), '-');
        offsets_add(&ofs, O2(l2 , hits), O2(l2 , accesses), O2(l2 , misses), '-');
        offsets_add(&ofs, O1(ll ), O1(l2), 0, '&');
        offsets_add(&ofs, O1(lbw), O1(l2), 0, '&');
    } else if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
        perfs_add(0xf064, PERF_TYPE_RAW, O2(l2 , hits    ), "PMCx064.1111.0000");
        perfs_add(0x0864, PERF_TYPE_RAW, O2(l2 , misses  ), "PMCx064.0000.1000");
        perfs_add(0x0729, PERF_TYPE_RAW, O2(l1d, accesses), "PMCx029.0000.0111");
        offsets_add(&ofs, O2(l2 , accesses), O2(l2 , misses  ), O2(l2 , hits  ), '+');
        offsets_add(&ofs, O2(l1d, misses  ), O2(l2 , accesses),               0, '=');
        offsets_add(&ofs, O2(l1d, hits    ), O2(l1d, accesses), O2(l1d, misses), '-');
        offsets_add(&ofs, O1(ll ), O1(l2), 0, '&');
        offsets_add(&ofs, O1(lbw), O1(l2), 0, '&');
    } else if (tp->vendor == VENDOR_ARM) {
        // Arm Architecture Reference Manual for A-profile architecture
        perfs_add(L1_LDA, PERF_TYPE_HW, O2(l1d, accesses), "PERF_L1D_LOAD_ACCESS");
        perfs_add(L1_LDM, PERF_TYPE_HW, O2(l1d, misses), "PERF_L1D_LOAD_MISS");
        perfs_add(0x0016, PERF_TYPE_RAW, O2(l2, accesses), "L2D_CACHE");
        perfs_add(0x0017, PERF_TYPE_RAW, O2(l2, misses), "L2D_CACHE_REFILL");
        perfs_add(0x0018, PERF_TYPE_RAW, O2(l2, lines_out), "L2D_CACHE_WB");
        offsets_add(&ofs, O2(l1d, hits), O2(l1d, accesses), O2(l1d, misses), '-');
        offsets_add(&ofs, O2(l2, hits), O2(l2, accesses), O2(l2, misses), '-');
        offsets_add(&ofs, O1(ll ), O1(l2), 0, '&');
        offsets_add(&ofs, O1(lbw), O1(l2), 0, '&');
    } else {
        perfs_add(L1_LDA, PERF_TYPE_HW, O2(l1d, accesses), "PERF_L1D_LOAD_ACCESS");
        perfs_add(L1_LDM, PERF_TYPE_HW, O2(l1d, misses  ), "PERF_L1D_LOAD_MISS"  );
        offsets_add(&ofs, O2(l1d, hits    ), O2(l1d, accesses), O2(l1d, misses), '-');
        offsets_add(&ofs, O2(l2 , accesses), O2(l1d, misses  ),               0, '=');
        offsets_add(&ofs, O1(ll ), O1(l1d), 0, '&');
        offsets_add(&ofs, O1(lbw), O1(l3 ), 0, '&'); // L3 will be 0, so it is ok
    }
    if (!perfs_count_opened_fds()) {
        debug("All PERFs failed to open");
        return;
    }
    apis_put(ops->get_info, cache_perf_get_info);
    apis_put(ops->init, cache_perf_init);
    apis_put(ops->dispose, cache_perf_dispose);
    apis_put(ops->read, cache_perf_read);
    apis_put(ops->internals_tostr, cache_perf_internals_tostr);
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
    for (i = 0; i < perfs_count; ++i) {
        if (perfs[i].fd >= 0) {
            perf_start(&perfs[i]);
        }
    }
    ++started;
    return EAR_SUCCESS;
}

state_t cache_perf_dispose()
{
    int i;
    if (started) {
        for (i = 0; i < perfs_count; ++i) {
            if (perfs[i].fd >= 0) {
                perf_close(&perfs[i]);
            }
        }
    }
    started     = 0;
    perfs_count = 0;
    return EAR_SUCCESS;
}

state_t cache_perf_read(cache_t *ca)
{
    void *p = (void *) ca;
    llong value;
    state_t s;
    int i;
    // Cleaning
    memset(ca, 0, sizeof(cache_t));
    // Reading
    timestamp_get(&ca->time);
    for (i = 0; i < perfs_count; ++i) {
        if (perfs[i].fd >= 0) {
            value = 0;
            if (state_fail(s = perf_read(&perfs[i], &value))) {
                return s;
            }
            offsets_calc_one(&ofs, p, i, value, 0LLU);
        }
    }
    offsets_calc_all(&ofs, p);
#if SHOW_DEBUGS
    cache_data_print(ca, 0.0, fderr);
#endif
    return EAR_SUCCESS;
}

void cache_perf_internals_tostr(char *buffer, int length)
{
    int i, c;
    for (i = c = 0; i < perfs_count; ++i) {
        c += sprintf(&buffer[c], "%s: %s\n", perfs[i].event_name, (perfs[i].fd > 0) ? "working" : "not-working");
    }
}