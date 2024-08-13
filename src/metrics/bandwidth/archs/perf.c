/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

//#define SHOW_DEBUGS 1

#include <common/system/time.h>
#include <common/output/debug.h>
#include <metrics/common/perf.h>
#include <metrics/bandwidth/archs/perf.h>

typedef struct meta_s {
    perf_t perf;
    cchar  *desc;
} meta_t;

static uint        counter;
static uint        started;
static meta_t      meta[5];

void set_event(ullong event, uint type, cchar *desc)
{
    if (state_fail(perf_open(&meta[counter].perf, &meta[0].perf, 0, type, event))) {
        debug("perf_open returned: %s", state_msg);
        return;
    }
    counter++;
}

state_t bwidth_perf_load(topology_t *tp, bwidth_ops_t *ops)
{
    if (tp->vendor == VENDOR_ARM) {
        set_event(0x19, PERF_TYPE_RAW, "BUS_ACCESS");
    }
    if (!counter) {
        return EAR_ERROR;
    }
    apis_put(ops->count_devices,   bwidth_perf_count_devices);
    apis_put(ops->get_granularity, bwidth_perf_get_granularity);
    apis_put(ops->init,            bwidth_perf_init);
    apis_put(ops->dispose,         bwidth_perf_dispose);
    apis_put(ops->read,            bwidth_perf_read);
    debug("Loaded PERF")
    return EAR_SUCCESS;
}

state_t bwidth_perf_count_devices(ctx_t *c, uint *devs_count)
{
    *devs_count = 1+1;
    return EAR_SUCCESS;
}

state_t bwidth_perf_get_granularity(ctx_t *c, uint *granularity)
{
    *granularity = GRANULARITY_CORE;
    return EAR_SUCCESS;
}

state_t bwidth_perf_init(ctx_t *c)
{
    if (!started) {
        perf_start(&meta[0].perf);
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
    ullong values[5];
    state_t s;
    int i;
    // Cleaning
    memset(bw, 0, sizeof(bwidth_t));
    // Reading
    timestamp_get(&bw[1].time);
    if (state_fail(s = perf_read(&meta[0].perf, (llong *) values))) {
        return s;
    }
    for (i = 0; i < counter; ++i) {
        bw[i].cas = values[i];
        debug("values[%d]: %llu (%s)", i, values[i], meta[i].desc);
    }
    return EAR_SUCCESS;
}
