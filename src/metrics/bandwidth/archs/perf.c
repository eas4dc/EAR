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

static uint started;
static perf_t *perfs;
static uint perfs_alloc_count;
static uint perfs_count;
static double castob; // cas to bytes

static void set_alloc(uint new_count)
{
    if (new_count <= perfs_alloc_count) {
        return;
    }
    if (perfs_alloc_count != 0) {
        perfs = realloc(perfs, new_count * sizeof(perf_t));
        memset(&perfs[perfs_alloc_count], 0, (new_count - perfs_alloc_count) * sizeof(perf_t));
    } else {
        perfs = calloc(new_count, sizeof(perf_t));
    }
    perfs_alloc_count = new_count;
}

static void set_event(ullong event, uint type, cchar *event_name)
{
    if (state_fail(perf_open(&perfs[perfs_count], NULL, 0, type, event))) {
        debug("perf_open returned: %s", state_msg);
        return;
    }
    sprintf(perfs[perfs_count].event_name, "%s", event_name);
    perfs_count++;
}

void bwidth_perf_load(topology_t *tp, bwidth_ops_t *ops)
{
    // Common configuration
    castob = tp->cache_line_size;
    if (tp->vendor == VENDOR_INTEL) {
        // In Sapphire Rapids (4 IMCs and 2 channels per IMC per socket), we
        // found the event files cas_count_read and cas_count_write. But
        // instead of opening two different events per pmu (uncore_imc_0...),
        // which is possible because there are 4 counters per pmu, we set the
        // read+write event which we know it is 0xff05.
        if (tp->model >= MODEL_SAPPHIRE_RAPIDS) {
            if (state_fail(perf_open_files(&perfs, "uncore_imc_%d", "0xff05", &perfs_count))) {
            }
        }
        // Ice Lake type. 4 IMCs and 2 Channels per IMC. In case of two sockets,
        // you will see 4 IMCs only, and two CPUs in cpumask. But, Free Running
        // Counters are per socket, so you can read the total data transferred
        // just reading a Free Running Counter.
        else if (tp->model >= MODEL_ICELAKE_X) {
            if (state_fail(perf_open_files(&perfs, "uncore_imc_free_running_0", "0x10ff", &perfs_count))) {
                if (state_fail(perf_open_files(&perfs, "uncore_imc_free_running_0", "data_total", &perfs_count))) {
                }
            }
        }
    } else if (tp->vendor == VENDOR_ARM) {
        if (tp->model == MODEL_NEOVERSE_V2) {
            if (state_ok(
                    perf_open_files(&perfs, "nvidia_scf_pmu_%d", "cmem_rd_data,cmem_wr_total_bytes", &perfs_count))) {
                perf_set_scale(perfs, perfs_count, "cmem_wr_total_bytes", 1.0 / 32.0);
                castob = 32.0;
            }
        } else {
            set_alloc(1);
            set_event(0x19, PERF_TYPE_RAW, "BUS_ACCESS");
        }
    }
    if (perfs_count == 0) {
        return;
    }
    apis_put(ops->get_info, bwidth_perf_get_info);
    apis_put(ops->init, bwidth_perf_init);
    apis_put(ops->dispose, bwidth_perf_dispose);
    apis_put(ops->read, bwidth_perf_read);
    apis_put(ops->castob, bwidth_perf_castob);
    debug("Loaded PERF")
}

BWIDTH_F_GET_INFO(bwidth_perf_get_info)
{
    info->api         = API_PERF;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_IMC;
    info->devs_count  = perfs_count + 1;
}

state_t bwidth_perf_init(ctx_t *c)
{
    int i;
    if (!started) {
        for (i = 0; i < perfs_count; ++i) {
            perf_start(&perfs[i]);
        }
        ++started;
    }
    return EAR_SUCCESS;
}

state_t bwidth_perf_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t bwidth_perf_read(ctx_t *c, bwidth_t *bw)
{
    llong value;
    int i;
    // Cleaning
    memset(bw, 0, perfs_count * sizeof(bwidth_t));
    // Reading
    timestamp_get(&bw[perfs_count].time);
    for (i = 0; i < perfs_count; ++i) {
        if (state_ok(perf_read(&perfs[i], &value))) {
            if (perfs[i].scale != 1.0) {
                bw[i].cas = (ullong) (((double) value) * perfs[i].scale);
            } else {
                bw[i].cas = value;
            }
            debug("CAS: %llu", bw[i].cas);
        }
    }
    return EAR_SUCCESS;
}

BWIDTH_F_CASTOB(bwidth_perf_castob)
{
    return ((double) cas) * castob;
}
