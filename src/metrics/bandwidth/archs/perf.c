/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <common/system/time.h>
#include <metrics/bandwidth/archs/perf.h>
#include <metrics/common/perf.h>
#include <stdlib.h>

static perf_t *perfs;
static uint perfs_count;
static double castob; // cas to bytes

static void close_all()
{
    uint i;
    for (i = 0; i < perfs_count; ++i) {
        perf_close(&perfs[i]);
    }
    free(perfs);
}

static void set_event(ullong event, uint type, cchar *event_name)
{
    // Allocating
    perfs = realloc(perfs, (perfs_count + 1) * sizeof(perf_t));
    //
    if (state_fail(perf_open(&perfs[perfs_count], NULL, 0, type, event))) {
        debug("perf_open returned: %s", state_msg);
        return;
    }
    sprintf(perfs[perfs_count].event_name, "%s", event_name);
    perfs_count++;
}

BWIDTH_F_LOAD(perf)
{
    int i;

    // Do not open PERF if an API is already loaded
    if (api_already_loaded(ops)) {
        return;
    }
    // Common configuration
    castob = tp->cache_line_size;
    // In Sapphire Rapids (4 IMCs and 2 channels per IMC per socket), we
    // found the event files cas_count_read and cas_count_write. But
    // instead of opening two different events per pmu (uncore_imc_0...),
    // which is possible because there are 4 counters per pmu, we set the
    // read+write event which we know it is 0xff05.
    if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_SAPPHIRE_RAPIDS) {
        if (state_fail(perf_open_files(&perfs, "uncore_imc_%d", "0xff05", &perfs_count))) {
        }
    }
    // Ice Lake type. 4 IMCs and 2 Channels per IMC. In case of two sockets,
    // you will see 4 IMCs only, and two CPUs in cpumask. But, Free Running
    // Counters are per socket, so you can read the total data transferred
    // just reading a Free Running Counter.
    else if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_ICELAKE_X) {
        if (state_fail(perf_open_files(&perfs, "uncore_imc_free_running_0", "0x10ff", &perfs_count))) {
            if (state_fail(perf_open_files(&perfs, "uncore_imc_free_running_0", "data_total", &perfs_count))) {
            }
        }
    } else if (tp->vendor == VENDOR_ARM && tp->model == MODEL_NEOVERSE_V2) {
        if (state_ok(perf_open_files(&perfs, "nvidia_scf_pmu_%d", "cmem_rd_data,cmem_wr_total_bytes", &perfs_count))) {
            perf_set_scale(perfs, perfs_count, "cmem_wr_total_bytes", 1.0 / 32.0);
            castob = 32.0;
        }
    } else if (tp->vendor == VENDOR_ARM) {
        set_event(0x19, PERF_TYPE_RAW, "BUS_ACCESS");
    }
    if (perfs_count == 0) {
        return;
    }
    for (i = 0; i < perfs_count; ++i) {
        if (state_fail(perf_start(&perfs[i]))) {
            close_all();
        }
    }
    apis_put(ops->unload, bwidth_perf_unload);
    apis_put(ops->get_info, bwidth_perf_get_info);
    apis_put(ops->read, bwidth_perf_read);
    apis_put(ops->castob, bwidth_perf_castob);
    debug("Loaded PERF")
}

BWIDTH_F_UNLOAD(perf)
{
    close_all();
}

BWIDTH_F_GET_INFO(perf)
{
    info->api         = API_PERF;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_IMC;
    info->devs_count  = perfs_count + 1;
}

BWIDTH_F_READ(perf)
{
    llong value;
    int i;
    // Cleaning
    memset(b, 0, perfs_count * sizeof(bwidth_t));
    // Reading
    timestamp_get(&b[perfs_count].time);
    for (i = 0; i < perfs_count; ++i) {
        if (state_ok(perf_read(&perfs[i], &value))) {
            if (perfs[i].scale != 1.0) {
                b[i].cas = (ullong) (((double) value) * perfs[i].scale);
            } else {
                b[i].cas = value;
            }
            debug("CAS: %llu", b[i].cas);
        }
    }
    return EAR_SUCCESS;
}

BWIDTH_F_CASTOB(perf)
{
    return ((double) cas) * castob;
}
