/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
// #define SHOW_DEBUGS 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <common/system/time.h>
#include <common/output/debug.h>
#include <common/math_operations.h>
#include <metrics/common/perf.h>
#include <metrics/flops/archs/perf.h>

typedef struct perfs_s {
    perf_t perfs[8];
    int offsets[8];
    double weights[8];
    uint perfs_count;
} perfs_t;

// Generic
static uint     vendor;
static uint     model;
static uint     family;
static uint     sve_bits;
static perfs_t *perfs;
static uint     perfs_count;
static uint     scope;
static uint     granularity;

static int add_perf(perfs_t *p, pid_t pid, uint cpu, ullong event, uint type, int offset, double weight, const char *event_desc)
{
    // Opening perf event (some events might be accepted but don't work)
    if (state_ok(perf_open_cpu(&p->perfs[p->perfs_count], NULL, pid, type, (ulong) event, 0, cpu))) {
        if (state_fail(perf_start(&p->perfs[p->perfs_count]))) {
            debug("perf_start() failed: %s", state_msg);
            perf_close(&p->perfs[p->perfs_count]);
            return 0;
        }
    } else {
        debug("perf_open() failed: %s", state_msg);
        return 0;
    }
    p->perfs[0].pid = (pid == 0)? getpid(): (pid == -1)? 1: pid;
    p->offsets[p->perfs_count] = offset;
    p->weights[p->perfs_count] = weight;
    strcpy(p->perfs[p->perfs_count].event_name, event_desc);
    p->perfs_count++;
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

static int add_events(perfs_t *p, uint pid, uint cpu)
{
    int ps = 0;
    if (vendor == VENDOR_INTEL && model >= MODEL_HASWELL_X) {
        ps += add_perf(p, pid, cpu, 0x02c7, PERF_TYPE_RAW, 0,  1.0, "FP_ARITH_INST_RETIRED.SCALAR_SINGLE     ");
        ps += add_perf(p, pid, cpu, 0x01c7, PERF_TYPE_RAW, 1,  1.0, "FP_ARITH_INST_RETIRED.SCALAR_DOUBLE     ");
        ps += add_perf(p, pid, cpu, 0x08c7, PERF_TYPE_RAW, 2,  4.0, "FP_ARITH_INST_RETIRED.128B_PACKED_SINGLE");
        ps += add_perf(p, pid, cpu, 0x04c7, PERF_TYPE_RAW, 3,  2.0, "FP_ARITH_INST_RETIRED.128B_PACKED_DOUBLE");
        ps += add_perf(p, pid, cpu, 0x20c7, PERF_TYPE_RAW, 4,  8.0, "FP_ARITH_INST_RETIRED.256B_PACKED_SINGLE");
        ps += add_perf(p, pid, cpu, 0x10c7, PERF_TYPE_RAW, 5,  4.0, "FP_ARITH_INST_RETIRED.256B_PACKED_DOUBLE");
        ps += add_perf(p, pid, cpu, 0x80c7, PERF_TYPE_RAW, 6, 16.0, "FP_ARITH_INST_RETIRED.512B_PACKED_SINGLE");
        ps += add_perf(p, pid, cpu, 0x40c7, PERF_TYPE_RAW, 7,  8.0, "FP_ARITH_INST_RETIRED.512B_PACKED_DOUBLE");
    } else if (vendor == VENDOR_AMD && family >= FAMILY_ZEN) {
        if (family >= FAMILY_ZEN5) {
            // This ZEN5 register contains type selection (float and double). 0h
            // means all types, but this can be reworked next to Retired_FP_uOps
            // to count 512, 256 and 128 length uops.
            ps += add_perf(p, pid, cpu, 0x0f03, PERF_TYPE_RAW, 4, 1.0, "Retired_SSE_AVX_FLOPs");
        } else {
            // In theory, this event counts 128 and 256 bits operations from SSE and
            // AVX FLOPs. But it can not distinguish between types. We are saving
            // the register readings in 256f by pure arbitrariness.
            ps += add_perf(p, pid, cpu, 0xff03, PERF_TYPE_RAW, 4, 1.0, "FpRetSseAvxOps");
        }
    } else if (vendor == VENDOR_ARM && model == MODEL_FX1000) {
        // We can mix these offsets because share the same weight. FIXED events
        // combines floats, doubles and also SIMD NEON.
        ps += add_perf(p, pid, cpu, 0x80c3, PERF_TYPE_RAW, 2, 1.0, "FP_HP_FIXED_OPS_SPEC");
        ps += add_perf(p, pid, cpu, 0x80c5, PERF_TYPE_RAW, 2, 1.0, "FP_SP_FIXED_OPS_SPEC");
        ps += add_perf(p, pid, cpu, 0x80c7, PERF_TYPE_RAW, 3, 1.0, "FP_DP_FIXED_OPS_SPEC");
        ps += add_perf(p, pid, cpu, 0x80c2, PERF_TYPE_RAW, 6, ((double) sve_bits) / 128.0, "FP_HP_SCALE_OPS_SPEC");
        ps += add_perf(p, pid, cpu, 0x80c4, PERF_TYPE_RAW, 6, ((double) sve_bits) / 128.0, "FP_SP_SCALE_OPS_SPEC");
        ps += add_perf(p, pid, cpu, 0x80c6, PERF_TYPE_RAW, 7, ((double) sve_bits) / 128.0, "FP_DP_SCALE_OPS_SPEC");
    } else if (vendor == VENDOR_ARM && sve_bits && model == MODEL_NEOVERSE_V2) {
        // FIXED also counts VFP_SPEC operations.
        ps += add_perf(p, pid, cpu, 0x80C1, PERF_TYPE_RAW, 2, 1.0, "FP_FIXED_OPS_SPEC");
        ps += add_perf(p, pid, cpu, 0x80c0, PERF_TYPE_RAW, 6, ((double) sve_bits) / 128.0, "FP_SCALE_OPS_SPEC");
    } else if (vendor == VENDOR_ARM && sve_bits && model != MODEL_NEOVERSE_V2) {
        // FIXED also counts VFP_SPEC operations.
        ps += add_perf(p, pid, cpu, 0x80c3, PERF_TYPE_RAW, 2, 1.0, "FP_FIXED_OPS_SPEC");
        ps += add_perf(p, pid, cpu, 0x80c2, PERF_TYPE_RAW, 6, ((double) sve_bits) / 128.0, "FP_SCALE_OPS_SPEC");
    } else if (vendor == VENDOR_ARM) {
        // ASE_SPEC is multiplied by x4 because we are counting float32x4_t, the
        // mean value of float16x8_t, float32x4_t and float64x2_t.
        ps += add_perf(p, pid, cpu, 0x0075, PERF_TYPE_RAW, 0, 1.0, "VFP_SPEC");
        ps += add_perf(p, pid, cpu, 0x0074, PERF_TYPE_RAW, 2, 4.0, "ASE_SPEC");
    }
    if (ps == 0) {
        // Destroy the offsets
        return_reprint(EAR_ERROR, "Failed all perf events: %s", state_msg);
    }
    return EAR_SUCCESS;
}

FLOPS_F_LOAD(perf)
{
    int i;

    vendor   = tp->vendor;
    model    = tp->model;
    family   = tp->family;
    sve_bits = tp->sve_bits;
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
    apis_put(ops->unload         , flops_perf_unload);
    apis_put(ops->update         , flops_perf_update);
    apis_put(ops->get_info       , flops_perf_get_info);
    apis_put(ops->read           , flops_perf_read);
    apis_put(ops->data_diff      , flops_perf_data_diff);
    apis_put(ops->internals_tostr, flops_perf_internals_tostr);
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

FLOPS_F_UNLOAD(perf)
{
    if (perfs == NULL) {
        return;
    }
    perfs_clean();
    free(perfs);
    perfs = NULL;
    perfs_count = 0;
}

FLOPS_F_UPDATE(perf)
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

FLOPS_F_GET_INFO(perf)
{
    info->api         = API_PERF;
    info->devs_count  = perfs_count;
    info->scope       = scope;
    info->granularity = granularity;
}

FLOPS_F_READ(perf)
{
    ullong *p = NULL;
    ullong value;
    state_t s;
    int i, j;

    // Cleaning flops_t structure
    memset(fl, 0, sizeof(flops_t) * perfs_count);
    // Getting time
    timestamp_get(&fl[0].time);
    // Reading
    for (i = 0; i < perfs_count; ++i) {
        if (perfs[i].perfs[0].fd <= 0) {
            continue;
        }
        p = (ullong *) &fl[i].f64;
        fl[i].pid = perfs[i].perfs[0].pid;
        #if SHOW_DEBUGS
        dprintf(debug_channel, "%s:%s:%d: d%d ", __FILE__, __FUNCTION__, __LINE__, i);
        #endif
        for (j = 0; j < perfs[i].perfs_count; ++j) {
            // We assume 0 means unopened file descriptor
            if (perfs[i].perfs[j].fd <= 0) {
                continue;
            }
            value = 0LL;
            if (state_fail(s = perf_read(&perfs[i].perfs[j], (long long *) &value))) {
                debug("perf_read failed: %s", state_msg);
                // return s;
            }
            p[j] = value;
            #if SHOW_DEBUGS
            dprintf(debug_channel, "%14lld ", value);
            #endif
        }
        #if SHOW_DEBUGS
        dprintf(debug_channel, "\n");
        #endif
    }
    return EAR_SUCCESS;
}

FLOPS_F_DATA_DIFF(perf)
{
    static flops_t *flA = NULL; // Auxiliar in case flD is NULL
    ullong *p1          = NULL;
    ullong *p2          = NULL;
    ullong *pD          = NULL;
    double gflops       = 0.0;
    double gflops_i     = 0.0;
    double secs         = 0.0;
    int i;

    if (flD != NULL) {
        memset(flD, 0, sizeof(flops_t) * perfs_count);
    } else if (flA == NULL) {
        flA = calloc(perfs_count, sizeof(flops_t));
    } else {
        memset(flA, 0, sizeof(flops_t) * perfs_count);
    }
    // Reading milliseconds and converting to seconds for decimals
    if ((secs = timestamp_fdiff(&fl2[0].time, &fl1[0].time, TIME_SECS, TIME_MSECS)) == 0LLU) {
        // If no time passed, then flops difference an Gigaflops are zero.
        return;
    }
    // Instruction differences (PERF registers are 64 bits long)
    for (i = 0; i < perfs_count; ++i) {
        if (fl2[i].pid == 0 || fl1[i].pid == 0) {
            continue;
        }
        p1 = (ullong *) &fl1[i].f64;
        p2 = (ullong *) &fl2[i].f64;
        pD = (flD != NULL)? (ullong *) &flD[i].f64: (ullong *) &flA[i].f64;
        // Is overflow_zero because sometimes values at time X are under
        // values al time Y, being X greater than Y. This is because of
        // perf's multiplexing process.
        pD[perfs[i].offsets[0]] = (ullong) (((double) overflow_zeros_u64(p2[0], p1[0])) * perfs[i].weights[0]);
        pD[perfs[i].offsets[1]] = (ullong) (((double) overflow_zeros_u64(p2[1], p1[1])) * perfs[i].weights[1]);
        pD[perfs[i].offsets[2]] = (ullong) (((double) overflow_zeros_u64(p2[2], p1[2])) * perfs[i].weights[2]);
        pD[perfs[i].offsets[3]] = (ullong) (((double) overflow_zeros_u64(p2[3], p1[3])) * perfs[i].weights[3]);
        pD[perfs[i].offsets[4]] = (ullong) (((double) overflow_zeros_u64(p2[4], p1[4])) * perfs[i].weights[4]);
        pD[perfs[i].offsets[5]] = (ullong) (((double) overflow_zeros_u64(p2[5], p1[5])) * perfs[i].weights[5]);
        pD[perfs[i].offsets[6]] = (ullong) (((double) overflow_zeros_u64(p2[6], p1[6])) * perfs[i].weights[6]);
        pD[perfs[i].offsets[7]] = (ullong) (((double) overflow_zeros_u64(p2[7], p1[7])) * perfs[i].weights[7]);
        gflops_i = ((double) pD[0]) + ((double) pD[1]) + ((double) pD[2]) + ((double) pD[3]) + ((double) pD[4]) +
                   ((double) pD[5]) + ((double) pD[6]) + ((double) pD[7]);
        gflops_i = (gflops_i / secs) / ((double) 1E9);
        gflops  += (gflops_i);
        if (flD != NULL) flD[i].pid    = fl2[i].pid;
        if (flD != NULL) flD[i].gflops = gflops_i;
        if (flD != NULL) flD[i].secs   = secs;
    }
    // gflops = (gflops / secs) / ((double) 1E9);
    if (gfs != NULL) {
        *gfs = gflops;
    }
}

FLOPS_F_INTERNALS_TOSTR(perf)
{
    cchar *offstr[] = {"F64 ", "D64 ", "F128", "D128", "F256", "D256", "F512", "D512"};
    cchar *state[]  = {"failed", "loaded"};
    int i, c;
    for (i = c = 0; i < perfs[0].perfs_count; ++i) {
        c += sprintf(&buffer[c], "FLOPS %s: %s event %s * %-2.2lf\n", offstr[perfs[0].offsets[i]],
                     state[perfs[0].perfs[i].fd > 0], perfs[0].perfs[i].event_name, perfs[0].weights[i]);
    }
}
