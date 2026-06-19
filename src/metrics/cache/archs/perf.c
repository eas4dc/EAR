/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* clang-format off */
// #define SHOW_DEBUGS 1
#include <common/system/time.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/common/perf.h>
#include <metrics/common/offsets.h>
#include <metrics/cache/archs/perf.h>

#define PERF_TYPE_HW PERF_TYPE_HW_CACHE
#define L1_LDM       PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)
#define L1_LDA       PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)

typedef struct perfs_s {
    perf_t perfs[6];
    uint perfs_count;
    ofops_t ofs;
} perfs_t;

// Generic
static uint     vendor;
static uint     model;
static uint     family;
static perfs_t *perfs;
static uint     perfs_count;
static uint     scope;
static uint     granularity;

static int perf_add(perfs_t *p, pid_t pid, uint cpu, ullong event, uint type, ulong offset_result, const char *event_desc)
{
    offsets_add(&p->ofs, offset_result, NO_OFFSET, NO_OFFSET, '=');
    if (state_ok(perf_open_cpu(&p->perfs[p->perfs_count], NULL, pid, type, event, 0, cpu))) {
        if (state_fail(perf_start(&p->perfs[p->perfs_count]))) {
            debug("perf_start() failed: %s", state_msg);
            perf_close(&p->perfs[p->perfs_count]);
            return 0;
        }
    } else {
        debug("perf_open() failed: %s", state_msg);
        return 0;
    }
    // Translation of PIDs from PERF to this API
    // pid -1 (no pid)       => pid 1 (system process that means 'node')
    // pid  0 (own pid)      => getpid()
    // pid >0 (specific pid) => pid >0
    // pid  0 (from calloc)  => pid  0 (not valid pid)
    p->perfs[0].pid = (pid == 0)? getpid(): (pid == -1)? 1: pid;
    strcpy(p->perfs[p->perfs_count].event_name, event_desc);
    ++p->perfs_count;
    return 1;
}

static int perfs_count_opened_fds()
{
    int i, j, k;
    for (i = k = 0; i < perfs_count; ++i) {
        for (j = 0; j < perfs[i].perfs_count; ++j) {
            // We assume 0 means unopened file descriptor
            k += (perfs[i].perfs[j].fd > 0);
        }
    }
    return k;
}

#define O2(l, v) (((int) offsetof(cache_t, l)) + ((int) offsetof(cache_level_t, v)))
#define O1(l)    (((int) offsetof(cache_t, l)))

static int add_events(perfs_t *p, uint pid, uint cpu)
{
    int ps = 0;

    if (vendor == VENDOR_INTEL && model >= MODEL_SKYLAKE_X) {
        ps += perf_add(p, pid, cpu, 0x01d1, PERF_TYPE_RAW, O2(l1d, hits    ), "MEM_LOAD_RETIRED.L1_HIT       ");
        ps += perf_add(p, pid, cpu, 0x08d1, PERF_TYPE_RAW, O2(l1d, misses  ), "MEM_LOAD_RETIRED.L1_MISS      ");
        ps += perf_add(p, pid, cpu, 0xe724, PERF_TYPE_RAW, O2(l2 , accesses), "L2_RQSTS.ALL_DEMAND_REFERENCES");
        ps += perf_add(p, pid, cpu, 0x2724, PERF_TYPE_RAW, O2(l2 , misses  ), "L2_RQSTS.ALL_DEMAND_MISS      ");
        ps += perf_add(p, pid, cpu, 0x04d1, PERF_TYPE_RAW, O2(l3 , hits    ), "MEM_LOAD_RETIRED.L3_HIT       ");
        ps += perf_add(p, pid, cpu, 0x20d1, PERF_TYPE_RAW, O2(l3 , misses  ), "MEM_LOAD_RETIRED.L3_MISS      ");
        offsets_add(&p->ofs, O2(l1d, accesses), O2(l1d, hits    ), O2(l1d, misses), '+');
        offsets_add(&p->ofs, O2(l2 , hits    ), O2(l2 , accesses), O2(l2 , misses), '-');
        offsets_add(&p->ofs, O2(l3 , accesses), O2(l3 , hits    ), O2(l2 , misses), '+');
        offsets_add(&p->ofs, O1(ll ), O1(l3), 0, '&');
        offsets_add(&p->ofs, O1(lbw), O1(l2), 0, '&');
    } else if (vendor == VENDOR_INTEL && model >= MODEL_BROADWELL_X) {
        ps += perf_add(p, pid, cpu, L1_LDA, PERF_TYPE_HW , O2(l1d, accesses), "PERF_L1D_LOAD_ACCESS          ");
        ps += perf_add(p, pid, cpu, L1_LDM, PERF_TYPE_HW , O2(l1d, misses  ), "PERF_L1D_LOAD_MISS            ");
        ps += perf_add(p, pid, cpu, 0xe724, PERF_TYPE_RAW, O2(l2 , accesses), "L2_RQSTS.ALL_DEMAND_REFERENCES");
        ps += perf_add(p, pid, cpu, 0x2724, PERF_TYPE_RAW, O2(l2 , misses  ), "L2_RQSTS.ALL_DEMAND_MISS      ");
        offsets_add(&p->ofs, O2(l1d, hits), O2(l1d, accesses), O2(l1d, misses), '-');
        offsets_add(&p->ofs, O2(l2 , hits), O2(l2 , accesses), O2(l2 , misses), '-');
        offsets_add(&p->ofs, O1(ll ), O1(l2), 0, '&');
        offsets_add(&p->ofs, O1(lbw), O1(l2), 0, '&');
    } else if (vendor == VENDOR_AMD && family >= FAMILY_ZEN) {
        ps += perf_add(p, pid, cpu, 0xf064, PERF_TYPE_RAW, O2(l2 , hits    ), "PMCx064.1111.0000");
        ps += perf_add(p, pid, cpu, 0x0864, PERF_TYPE_RAW, O2(l2 , misses  ), "PMCx064.0000.1000");
        ps += perf_add(p, pid, cpu, 0x0729, PERF_TYPE_RAW, O2(l1d, accesses), "PMCx029.0000.0111");
        offsets_add(&p->ofs, O2(l2 , accesses), O2(l2 , misses  ), O2(l2, hits)   , '+');
        offsets_add(&p->ofs, O2(l1d, misses  ), O2(l2 , accesses),               0, '=');
        offsets_add(&p->ofs, O2(l1d, hits    ), O2(l1d, accesses), O2(l1d, misses), '-');
        offsets_add(&p->ofs, O1(ll ), O1(l2), 0, '&');
        offsets_add(&p->ofs, O1(lbw), O1(l2), 0, '&');
    } else if (vendor == VENDOR_ARM) {
        // Arm Architecture Reference Manual for A-profile architecture
        ps += perf_add(p, pid, cpu, L1_LDA, PERF_TYPE_HW , O2(l1d, accesses ), "PERF_L1D_LOAD_ACCESS");
        ps += perf_add(p, pid, cpu, L1_LDM, PERF_TYPE_HW , O2(l1d, misses   ), "PERF_L1D_LOAD_MISS  ");
        ps += perf_add(p, pid, cpu, 0x0016, PERF_TYPE_RAW, O2(l2 , accesses ), "L2D_CACHE           ");
        ps += perf_add(p, pid, cpu, 0x0017, PERF_TYPE_RAW, O2(l2 , misses   ), "L2D_CACHE_REFILL    ");
        ps += perf_add(p, pid, cpu, 0x0018, PERF_TYPE_RAW, O2(l2 , lines_out), "L2D_CACHE_WB        ");
        offsets_add(&p->ofs, O2(l1d, hits), O2(l1d, accesses ), O2(l1d, misses), '-');
        offsets_add(&p->ofs, O2(l2 , hits), O2(l2 , accesses ), O2(l2 , misses), '-');
        offsets_add(&p->ofs, O1(ll ), O1(l2), 0, '&');
        offsets_add(&p->ofs, O1(lbw), O1(l2), 0, '&');
    } else {
        ps += perf_add(p, pid, cpu, L1_LDA, PERF_TYPE_HW, O2(l1d, accesses), "PERF_L1D_LOAD_ACCESS");
        ps += perf_add(p, pid, cpu, L1_LDM, PERF_TYPE_HW, O2(l1d, misses  ), "PERF_L1D_LOAD_MISS"  );
        offsets_add(&p->ofs, O2(l1d, hits    ), O2(l1d, accesses), O2(l1d, misses), '-');
        offsets_add(&p->ofs, O2(l2 , accesses), O2(l1d, misses  ),               0, '=');
        offsets_add(&p->ofs, O1(ll ), O1(l1d), 0, '&');
        offsets_add(&p->ofs, O1(lbw), O1(l3 ), 0, '&'); // L3 will be 0, so it is ok
    }
    if (ps == 0) {
        // Destroy de offsets
        return_print(EAR_ERROR, "Failed all perf events: %s", state_msg);
    }
    return EAR_SUCCESS;
}

CACHE_F_LOAD(perf)
{
    int i;

    vendor = tp->vendor;
    model  = tp->model;
    family = tp->family;
    if (SCOPE_IS(options, SCOPE_NODE)) {
        perfs_count = tp->cpu_count;
        perfs       = calloc(perfs_count, sizeof(perfs_t));
        scope       = SCOPE_NODE;
        granularity = GRANULARITY_CPU;
        // Event per CPU
        for (i = 0; i < perfs_count; ++i) {
            add_events(&perfs[i], -1, i);
        }
    } else if (SCOPE_IS(options, SCOPE_JOB)) {
        if (!perf_is_working()) {
            return;
        }
        perfs_count = tp->cpu_count * 3;
        perfs       = calloc(perfs_count, sizeof(perfs_t));
        scope       = SCOPE_JOB;
        granularity = GRANULARITY_PROCESS;
    } else {
        perfs_count = 1;
        perfs       = calloc(perfs_count, sizeof(perfs_t));
        scope       = SCOPE_PROCESS;
        granularity = GRANULARITY_PROCESS;
        add_events(&perfs[0], 0, -1);
    }
    if (scope != SCOPE_JOB && !perfs_count_opened_fds()) {
        return;
    }
    apis_put(ops->unload  , cache_perf_unload);
    apis_put(ops->update  , cache_perf_update);
    apis_put(ops->get_info, cache_perf_get_info);
    apis_put(ops->read    , cache_perf_read);
    apis_put(ops->internals_tostr, cache_perf_internals_tostr);
}

static void perf_remove(perfs_t *perf)
{
    int j;
    for (j = 0; j < perf->perfs_count; ++j) {
        // We assume 0 means unopened file descriptor
        if (perf->perfs[j].fd <= 0) {
            continue;
        }
        perf_close(&perf->perfs[j]);
    }
    memset(perf, 0, sizeof(perfs_t));
}

static state_t perfs_clean()
{
    int i;
    for (i = 0; i < perfs_count; ++i) {
        perf_remove(&perfs[i]);
    }
    return EAR_SUCCESS;
}

CACHE_F_UNLOAD(perf)
{
    if (perfs == NULL) {
        return;
    }
    perfs_clean();
    free(perfs);
    perfs = NULL;
    perfs_count = 0;
}

CACHE_F_UPDATE(perf)
{
    pid_t pid = (uint) ((ullong) value);
    int i, j = perfs_count;

    if (scope == SCOPE_JOB && option == UPD_PID_ADD) {
        for (i = 0; i < perfs_count; ++i) {
            if (perfs[i].perfs[0].pid == pid) {
                return_msg(EAR_ERROR, "PID already added");
            }
            j = (perfs[i].perfs[0].pid == 0 && i < j)? i: j;
        }
        if (j >= perfs_count) {
            return_msg(EAR_ERROR, "max number of PIDs reached")
        }
        return add_events(&perfs[j], pid, -1);
    } else if (scope == SCOPE_JOB && option == UPD_PID_REMOVE) {
        for (i = 0; i < perfs_count; ++i) {
            if (perfs[i].perfs[0].pid == pid) {
                perf_remove(&perfs[i]);
                return EAR_SUCCESS;
            }
        }
        return_msg(EAR_ERROR, "PID not found");
    } else if (scope == SCOPE_JOB && option == UPD_PIDS_CLEAN) {
        return perfs_clean();
    }
    return_msg(EAR_ERROR, "option not available");
}

void cache_perf_get_info(apinfo_t *info)
{
    info->api         = API_PERF;
    info->devs_count  = perfs_count;
    info->scope       = scope;
    info->granularity = granularity;
}

CACHE_F_READ(perf)
{
    llong value;
    state_t s;
    int i, j;

    for (i = 0; i < perfs_count; ++i) {
        if (perfs[i].perfs[0].pid == 0) {
            continue;
        }
        ca[i].pid = perfs[i].perfs[0].pid;
        timestamp_get(&ca[i].time);
        #if SHOW_DEBUGS
        dprintf(debug_channel, "%s:%s:%d: d%d ", __FILE__, __FUNCTION__, __LINE__, i);
        #endif
        for (j = 0; j < perfs[i].perfs_count; ++j) {
            // We assume 0 means unopened file descriptor
            if (perfs[i].perfs[j].fd <= 0) {
                continue;
            }
            value = 0LL;
            if (state_fail(s = perf_read(&perfs[i].perfs[j], &value))) {
                // debug("perf_read failed: %s", state_msg);
                //return s;
            }
            #if SHOW_DEBUGS
            dprintf(debug_channel, "%14lld ", value);
            #endif
            offsets_calc_one(&perfs[i].ofs, (void *) &ca[i], j, value, 0LLU);
        }
        offsets_calc(&perfs[i].ofs, (void *) &ca[i]);
        #if SHOW_DEBUGS
        dprintf(debug_channel, "\n");
        #endif
    }
    return EAR_SUCCESS;
}

void cache_perf_internals_tostr(char *buffer, int length)
{
    int i, j, b, w;
    // Forced to print 1 instead the whole devs_count
    for (i = b = 0; i < 1 && length > 0; ++i) {
        for (j = 0; j < perfs[i].perfs_count && length > 0; ++j) {
            w = snprintf(&buffer[b], length, "%s: %s\n",
                  perfs[i].perfs[j].event_name, (perfs[i].perfs[j].fd > 0) ? "working" : "not-working");
            b += w, length -= w;
        }
    }
}
