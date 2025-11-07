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

#include <common/output/debug.h>
#include <metrics/common/offsets.h>
#include <metrics/common/perf.h>
#include <metrics/cpi/archs/perf.h>

// Generic
static perf_t perfs[4];
static uint perfs_count;
static uint started;
static offset_ops_t ofs;

static void perfs_add(ullong event, uint type, ulong offset_result, const char *event_desc)
{
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

#define O2(l, v) (((int) offsetof(cpi_t, l)) + ((int) offsetof(stalls_t, v)))
#define O1(l)    (((int) offsetof(cpi_t, l)))

void cpi_perf_load(topology_t *tp, cpi_ops_t *ops)
{
    if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_ICELAKE_X) {
        perfs_add(PERF_COUNT_HW_CPU_CYCLES, PERF_TYPE_HARDWARE, O1(cycles), "PERF_COUNT_HW_CPU_CYCLES   ");
        perfs_add(PERF_COUNT_HW_INSTRUCTIONS, PERF_TYPE_HARDWARE, O1(instructions), "PERF_COUNT_HW_INSTRUCTIONS ");
        perfs_add(0x40004a3, PERF_TYPE_RAW, O2(stalls, memory), "CYCLE_ACTIVITY.STALLS_TOTAL");
    } else if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_BROADWELL_X) {
        perfs_add(PERF_COUNT_HW_CPU_CYCLES, PERF_TYPE_HARDWARE, O1(cycles), "PERF_COUNT_HW_CPU_CYCLES   ");
        perfs_add(PERF_COUNT_HW_INSTRUCTIONS, PERF_TYPE_HARDWARE, O1(instructions), "PERF_COUNT_HW_INSTRUCTIONS ");
        perfs_add(0x40004a3, PERF_TYPE_RAW, O2(stalls, memory), "CYCLE_ACTIVITY.STALLS_TOTAL");
        perfs_add(0x00001a2, PERF_TYPE_RAW, O2(stalls, resources), "RESOURCE_STALLS.ANY        ");
    } else if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
        // PMCx087 is inherited from Family 15h
        perfs_add(PERF_COUNT_HW_CPU_CYCLES, PERF_TYPE_HARDWARE, O1(cycles), "PERF_COUNT_HW_CPU_CYCLES     ");
        perfs_add(PERF_COUNT_HW_INSTRUCTIONS, PERF_TYPE_HARDWARE, O1(instructions), "PERF_COUNT_HW_INSTRUCTIONS   ");
        perfs_add(0x0000287, PERF_TYPE_RAW, O2(stalls, fetch_decode), "IC_FETCH_STALL.DECODING_QUEUE");
        perfs_add(0x0000187, PERF_TYPE_RAW, O2(stalls, resources), "IC_FETCH_STALL.BACK_PRESSURE ");
    } else {
        perfs_add(PERF_COUNT_HW_CPU_CYCLES, PERF_TYPE_HARDWARE, O1(cycles), "PERF_COUNT_HW_CPU_CYCLES     ");
        perfs_add(PERF_COUNT_HW_INSTRUCTIONS, PERF_TYPE_HARDWARE, O1(instructions), "PERF_COUNT_HW_INSTRUCTIONS   ");
        perfs_add(PERF_COUNT_HW_STALLED_CYCLES_FRONTEND, PERF_TYPE_HARDWARE, O2(stalls, fetch_decode),
                  "PERF_COUNT_HW_STALLS_FRONTEND");
        perfs_add(PERF_COUNT_HW_STALLED_CYCLES_BACKEND, PERF_TYPE_HARDWARE, O2(stalls, memory),
                  "PERF_COUNT_HW_STALLS_BACKEND ");
    }
    if (!perfs_count_opened_fds()) {
        debug("All PERFs failed to open");
        return;
    }
    apis_put(ops->get_info, cpi_perf_get_info);
    apis_put(ops->init, cpi_perf_init);
    apis_put(ops->dispose, cpi_perf_dispose);
    apis_put(ops->read, cpi_perf_read);
}

void cpi_perf_get_info(apinfo_t *info)
{
    info->api = API_PERF;
}

state_t cpi_perf_init()
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

state_t cpi_perf_dispose()
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

state_t cpi_perf_read(cpi_t *ci)
{
    void *p = (void *) ci;
    llong value;
    state_t s;
    int i;
    // Cleaning
    memset(ci, 0, sizeof(cpi_t));
    // Reading
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
    cpi_data_print(ci, 0.0, fderr);
#endif
    return EAR_SUCCESS;
}