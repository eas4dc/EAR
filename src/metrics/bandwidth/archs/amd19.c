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

#include <common/hardware/bithack.h>
#include <common/math_operations.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <metrics/bandwidth/archs/amd19.h>
#include <metrics/common/hsmp.h>
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t  lock = PTHREAD_MUTEX_INITIALIZER;
static topology_t       tp;
static uint             fun; // Address
static suscription_t   *sus;
static bwidth_t        *pool;
static double           line_size;

BWIDTH_F_LOAD(bwidth_amd19_load)
{
    uint reps[2] = {0, -1};
    uint args[1] = {-1};

    if (tpo->vendor != VENDOR_AMD || tpo->family < FAMILY_ZEN) {
        return_msg(, Generr.api_incompatible);
    }
    if (state_fail(hsmp_open(tpo, HSMP_RD))) {
        debug("hsmp_open failed: %s", state_msg);
        return;
    }
    // Testing if the function is compatible
    if (state_fail(hsmp_send(0, HSMP_GET_DDR_BANDWIDTH, args, reps))) {
        debug("hsmp_send failed: %s", state_msg);
        return;
    }
    if (state_fail(topology_select(tpo, &tp, TPSelect.socket, TPGroup.merge, 0))) {
        return;
    }
    // ZEN3
    fun       = HSMP_GET_DDR_BANDWIDTH;
    line_size = (double) tpo->cache_line_size;
    //
    pool = calloc(tp.cpu_count + 1, sizeof(bwidth_t));

    apis_put(ops->get_info, bwidth_amd19_get_info);
    apis_put(ops->init    , bwidth_amd19_init);
    apis_put(ops->dispose , bwidth_amd19_dispose);
    apis_put(ops->read    , bwidth_amd19_read);
    debug("Loaded AMD19");
}

BWIDTH_F_GET_INFO(bwidth_amd19_get_info)
{
    info->api         = API_AMD19;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = tp.cpu_count + 1;
}

static state_t multiread(bwidth_t *bws)
{
    uint args[1] = {-1};
    uint reps[2] = {0, -1};
    timestamp_t time;
    double secs;
    double fcas;
    int sock;

    // Getting old time
    time = bws[tp.cpu_count].time;
    // Getting new time
    timestamp_get(&bws[tp.cpu_count].time);
    // Working in seconds with the precission of USECS
    secs = timestamp_fdiff(&bws[tp.cpu_count].time, &time, TIME_SECS, TIME_USECS);
    // Reading HSMP
    for (sock = 0; sock < tp.cpu_count; ++sock) {
        hsmp_send(sock, fun, args, reps);
        // Converting GB/s to GBytes since last read
        fcas = (double) getbits32(reps[0], 19, 8);
        fcas *= (double) 1E9;
        fcas *= (double) secs;
        fcas /= (double) line_size;
        bws[sock].cas += (ullong) fcas;
        debug("HSMP returned %u in %lf secs, or %0.3lf CAS", getbits32(reps[0], 19, 8), secs, fcas);
    }
    return EAR_SUCCESS;
}

static state_t multipool(void *something)
{
    while (pthread_mutex_trylock(&lock))
        ;
    multiread(pool);
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

BWIDTH_F_INIT(bwidth_amd19_init)
{
    // Init
    timestamp_get(&pool[tp.cpu_count].time);
    // Pool suscription
    sus             = suscription();
    sus->call_init  = NULL;
    sus->call_main  = multipool;
    sus->time_relax = 5000;
    sus->time_burst = 5000;
    return sus->suscribe(sus);
}

BWIDTH_F_DISPOSE(bwidth_amd19_dispose)
{
    return EAR_SUCCESS;
}

BWIDTH_F_READ(bwidth_amd19_read)
{
    // Update pool
    multipool(NULL);
    while (pthread_mutex_trylock(&lock))
        ;
    memcpy(bws, pool, sizeof(bwidth_t) * (tp.cpu_count + 1));
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}
