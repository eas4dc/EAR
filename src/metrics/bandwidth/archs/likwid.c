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

#include <common/math_operations.h>
#include <common/output/debug.h>
#include <common/system/user.h>
#include <metrics/bandwidth/archs/likwid.h>
#include <metrics/common/likwid.h>
#include <stdlib.h>

static char *ice_evs           = "CAS_COUNT_ALL:MBOX0C0,"
                                 "CAS_COUNT_ALL:MBOX1C0,"
                                 "CAS_COUNT_ALL:MBOX2C0,"
                                 "CAS_COUNT_ALL:MBOX3C0,"
                                 "CAS_COUNT_ALL:MBOX4C0,"
                                 "CAS_COUNT_ALL:MBOX5C0,"
                                 "CAS_COUNT_ALL:MBOX6C0,"
                                 "CAS_COUNT_ALL:MBOX7C0";
static char *ice_evs_names[8]  = {"CAS_COUNT_ALL:MBOX0C0", "CAS_COUNT_ALL:MBOX1C0", "CAS_COUNT_ALL:MBOX2C0",
                                  "CAS_COUNT_ALL:MBOX3C0", "CAS_COUNT_ALL:MBOX4C0", "CAS_COUNT_ALL:MBOX5C0",
                                  "CAS_COUNT_ALL:MBOX6C0", "CAS_COUNT_ALL:MBOX7C0"};
static char *sky_evs           = "CAS_COUNT_RD:MBOX0C1,"
                                 "CAS_COUNT_WR:MBOX0C2,"
                                 "CAS_COUNT_RD:MBOX1C1,"
                                 "CAS_COUNT_WR:MBOX1C2,"
                                 "CAS_COUNT_RD:MBOX2C1,"
                                 "CAS_COUNT_WR:MBOX2C2,"
                                 "CAS_COUNT_RD:MBOX3C1,"
                                 "CAS_COUNT_WR:MBOX3C2,"
                                 "CAS_COUNT_RD:MBOX4C1,"
                                 "CAS_COUNT_WR:MBOX4C2,"
                                 "CAS_COUNT_RD:MBOX5C1,"
                                 "CAS_COUNT_WR:MBOX5C2,"
                                 "CAS_COUNT_RD:MBOX6C1,"
                                 "CAS_COUNT_WR:MBOX6C2,"
                                 "CAS_COUNT_RD:MBOX7C1,"
                                 "CAS_COUNT_WR:MBOX7C2,";
static char *sky_evs_names[16] = {
    "CAS_COUNT_RD:MBOX0C1", "CAS_COUNT_WR:MBOX0C2", "CAS_COUNT_RD:MBOX1C1", "CAS_COUNT_WR:MBOX1C2",
    "CAS_COUNT_RD:MBOX2C1", "CAS_COUNT_WR:MBOX2C2", "CAS_COUNT_RD:MBOX3C1", "CAS_COUNT_WR:MBOX3C2",
    "CAS_COUNT_RD:MBOX4C1", "CAS_COUNT_WR:MBOX4C2", "CAS_COUNT_RD:MBOX5C1", "CAS_COUNT_WR:MBOX5C2",
    "CAS_COUNT_RD:MBOX6C1", "CAS_COUNT_WR:MBOX6C2", "CAS_COUNT_RD:MBOX7C1", "CAS_COUNT_WR:MBOX7C2"};

static uint ice_evs_count = 8;
static uint sky_evs_count = 16;
//
static topology_t tp;
static uint evs_count;
static char *evs;
static char **evs_names;
static likevs_t events;
static double *ctrs;
static uint ctrs_count;
static uint devs_count;
static uint cpus_count;
static ullong *ctrs_accum;

state_t bwidth_likwid_load(topology_t *tp_in, bwidth_ops_t *ops)
{
    state_t s;

    // This check is to avoid EARL procs which doesn't load EARD API,
    // try to load LIKWID library and daemon, which by now doesn't
    // work properly.
    if (!user_is_root()) {
        return_msg(EAR_ERROR, Generr.no_permissions);
    }
    if (tp_in->vendor != VENDOR_INTEL || tp_in->model < MODEL_SKYLAKE_X) {
        return_msg(EAR_ERROR, Generr.api_incompatible);
    }
    // If read is defined, it means that another API is loaded with permissions,
    // so it is preferred to avoid LIKWID's link and initialization.
    if (ops->read != NULL) {
        return_msg(EAR_ERROR, Generr.api_initialized);
    }
    // Total CPUs count
    cpus_count = tp_in->cpu_count;

    if (state_fail(s = topology_select(tp_in, &tp, TPSelect.socket, TPGroup.merge, 0))) {
        return s;
    }
    if (state_fail(s = likwid_init())) {
        debug("likwid_init failed: %s %d", state_msg, s);
        return s;
    }
    // Icelake
    if (tp_in->model >= MODEL_ICELAKE_X) {
        evs_count = ice_evs_count;
        evs_names = ice_evs_names;
        evs       = ice_evs;
    }
    // Skylake
    else {
        evs_count = sky_evs_count;
        evs_names = sky_evs_names;
        evs       = sky_evs;
    }
    if (state_fail(s = likwid_events_open(&events, &ctrs, &ctrs_count, evs, evs_count))) {
        debug("likwid_init failed: %s %d", state_msg, s);
        return s;
    }
    //
    ctrs_accum = calloc(ctrs_count, sizeof(ullong));
    devs_count = tp_in->socket_count * evs_count;
    // Debugging
    debug("sockets   : %u", tp.cpu_count);
    debug("total cpus: %u", cpus_count);
    debug("counters  : %u", devs_count);
    //
    apis_put(ops->get_info, bwidth_likwid_get_info);
    apis_put(ops->init, bwidth_likwid_init);
    apis_put(ops->dispose, bwidth_likwid_dispose);
    apis_put(ops->read, bwidth_likwid_read);
    return EAR_SUCCESS;
}

BWIDTH_F_GET_INFO(bwidth_likwid_get_info)
{
    info->api         = API_LIKWID;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_IMC;
    info->devs_count  = devs_count + 1;
}

state_t bwidth_likwid_init(ctx_t *c)
{
    // Likwid events open
    return EAR_SUCCESS;
}

state_t bwidth_likwid_dispose(ctx_t *c)
{
    // Likwid events close
    return EAR_SUCCESS;
}

state_t bwidth_likwid_count_devices(ctx_t *c, uint *devs_count_in)
{
    *devs_count_in = devs_count + 1;
    return EAR_SUCCESS;
}

state_t bwidth_likwid_read(ctx_t *c, bwidth_t *b)
{
    state_t s;
    int i, j, k;

    memset(b, 0, sizeof(bwidth_t) * (devs_count + 1));
    // Retrieving time in the position N
    timestamp_get(&b[devs_count].time);
    //
    if (state_fail(s = likwid_events_read(&events, ctrs))) {
        return s;
    }
    for (i = j = 0; i < tp.cpu_count; ++i) {
        for (k = 0; k < evs_count; ++k, ++j) {
            ctrs_accum[j] += (ullong) ctrs[(cpus_count * k) + tp.cpus[i].id];
            b[j].cas = ctrs_accum[j];
            debug("S%d%s: %llu", i, evs_names[k], b[j].cas);
        }
    }
    return EAR_SUCCESS;
}
