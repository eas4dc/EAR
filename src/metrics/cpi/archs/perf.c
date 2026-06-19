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
//#define SHOW_DEBUGS 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <common/output/debug.h>
#include <metrics/common/perf.h>
#include <metrics/common/offsets.h>
#include <metrics/cpi/archs/perf.h>

typedef struct perfs_s {
    perf_t perfs[4];
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

static int add_events(perfs_t *p, uint pid, uint cpu)
{
    #define O2(l, v) (((int) offsetof(cpi_t, l)) + ((int) offsetof(stalls_t, v)))
    #define O1(l)    (((int) offsetof(cpi_t, l)))
    int ps = 0;

    if (vendor == VENDOR_INTEL && model >= MODEL_ICELAKE_X) {
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_INSTRUCTIONS, PERF_TYPE_HARDWARE, O1(instructions)  , "PERF_COUNT_HW_INSTRUCTIONS ");
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_CPU_CYCLES  , PERF_TYPE_HARDWARE, O1(cycles)        , "PERF_COUNT_HW_CPU_CYCLES   ");
        ps += perf_add(p, pid, cpu, 0x40004a3                 , PERF_TYPE_RAW     , O2(stalls, memory), "CYCLE_ACTIVITY.STALLS_TOTAL");
    } else if (vendor == VENDOR_INTEL && model >= MODEL_BROADWELL_X) {
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_INSTRUCTIONS, PERF_TYPE_HARDWARE, O1(instructions)     , "PERF_COUNT_HW_INSTRUCTIONS ");
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_CPU_CYCLES  , PERF_TYPE_HARDWARE, O1(cycles)           , "PERF_COUNT_HW_CPU_CYCLES   ");
        ps += perf_add(p, pid, cpu, 0x40004a3                 , PERF_TYPE_RAW     , O2(stalls, memory)   , "CYCLE_ACTIVITY.STALLS_TOTAL");
        ps += perf_add(p, pid, cpu, 0x00001a2                 , PERF_TYPE_RAW     , O2(stalls, resources), "RESOURCE_STALLS.ANY        ");
    } else if (vendor == VENDOR_AMD && family >= FAMILY_ZEN) {
        // PMCx087 is inherited from Family 15h
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_INSTRUCTIONS, PERF_TYPE_HARDWARE, O1(instructions)        , "PERF_COUNT_HW_INSTRUCTIONS   ");
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_CPU_CYCLES  , PERF_TYPE_HARDWARE, O1(cycles)              , "PERF_COUNT_HW_CPU_CYCLES     ");
        ps += perf_add(p, pid, cpu, 0x0000287                 , PERF_TYPE_RAW     , O2(stalls, fetch_decode), "IC_FETCH_STALL.DECODING_QUEUE");
        ps += perf_add(p, pid, cpu, 0x0000187                 , PERF_TYPE_RAW     , O2(stalls, resources)   , "IC_FETCH_STALL.BACK_PRESSURE ");
    } else {
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_INSTRUCTIONS           , PERF_TYPE_HARDWARE, O1(instructions)        , "PERF_COUNT_HW_INSTRUCTIONS   ");
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_CPU_CYCLES             , PERF_TYPE_HARDWARE, O1(cycles)              , "PERF_COUNT_HW_CPU_CYCLES     ");
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND, PERF_TYPE_HARDWARE, O2(stalls, fetch_decode), "PERF_COUNT_HW_STALLS_FRONTEND");
        ps += perf_add(p, pid, cpu, PERF_COUNT_HW_STALLED_CYCLES_BACKEND , PERF_TYPE_HARDWARE, O2(stalls, memory)      , "PERF_COUNT_HW_STALLS_BACKEND ");
    }
    if (ps == 0) {
        // Destroy the offsets
        return_print(EAR_ERROR, "Failed all perf events: %s", state_msg);
    }
    return EAR_SUCCESS;
}

CPI_F_LOAD(perf)
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
    apis_put(ops->unload    , cpi_perf_unload    );
    apis_put(ops->update    , cpi_perf_update    );
    apis_put(ops->get_info  , cpi_perf_get_info  );
    apis_put(ops->read      , cpi_perf_read      );
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

CPI_F_UNLOAD(perf)
{
    if (perfs == NULL) {
        return;
    }
    perfs_clean();
    free(perfs);
    perfs = NULL;
    perfs_count = 0;
}

CPI_F_UPDATE(perf)
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

void cpi_perf_get_info(apinfo_t *info)
{
    info->api         = API_PERF;
    info->devs_count  = perfs_count;
    info->scope       = scope;
    info->granularity = granularity;
}

CPI_F_READ(perf)
{
    llong value;
    state_t s;
    int i, j;

    for (i = 0; i < perfs_count; ++i) {
        #if SHOW_DEBUGS
        dprintf(debug_channel, "DEV%d ", i);
        #endif
        cpi[i].pid = perfs[i].perfs[0].pid;
        for (j = 0; j < perfs[i].perfs_count; ++j) {
            // We assume 0 means unopened file descriptor
            if (perfs[i].perfs[j].fd <= 0) {
                continue;
            }
            value = 0LL;
            if (state_fail(s = perf_read(&perfs[i].perfs[j], &value))) {
                debug("FAILED");
                // return s;
            }
            offsets_calc_one(&perfs[i].ofs, (void *) &cpi[i], j, value, 0LLU);
            #if SHOW_DEBUGS
            dprintf(debug_channel, "%14lld ", value);
            #endif
        }
        offsets_calc(&perfs[i].ofs, (void *) &cpi[i]);
        #if SHOW_DEBUGS
        dprintf(debug_channel, "\n");
        #endif
    }
    return EAR_SUCCESS;
}
