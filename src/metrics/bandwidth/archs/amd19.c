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

#include <stdlib.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/math_operations.h>
#include <common/hardware/bithack.h>
#include <metrics/common/hsmp.h>
#include <metrics/bandwidth/archs/amd19.h>

static pthread_mutex_t  lock = PTHREAD_MUTEX_INITIALIZER;
static topology_t       tp_own;
static suscription_t   *sus = NULL;
static bwidth_t        *pool;
static double           line_size;

static state_t multiread(bwidth_t *bws)
{
    uint args[1] = {-1};
    uint reps[2] = {0, -1};
    timestamp_t time;
    double secs;
    double fcas;
    int sock;

    // Getting old time
    time = bws[tp_own.cpu_count].time;
    // Getting new time
    timestamp_get(&bws[tp_own.cpu_count].time);
    // Working in seconds with the precission of USECS
    secs = timestamp_fdiff(&bws[tp_own.cpu_count].time, &time, TIME_SECS, TIME_USECS);
    // Reading HSMP
    for (sock = 0; sock < tp_own.cpu_count; ++sock) {
        hsmp_send(sock, HSMP_GET_DDR_BANDWIDTH, args, reps);
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
    while (pthread_mutex_trylock(&lock));
    multiread(pool);
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

static void close_all()
{
    hsmp_close();
    topology_close(&tp_own);
    if (sus && sus->suscribe != NULL) {
        // It's protected inside
        monitor_unregister(sus);
    }
    if (pool != NULL) {
        free(pool);
        pool = NULL;
    }
}

BWIDTH_F_LOAD(amd19)
{
    uint reps[2] = {0, -1};
    uint args[1] = {-1};

    if (tp->vendor != VENDOR_AMD || tp->family < FAMILY_ZEN) {
        return_msg(, Generr.api_incompatible);
    }
    if (state_fail(hsmp_open(tp, HSMP_RD))) {
        debug("hsmp_open failed: %s", state_msg);
        close_all();
        return;
    }
    // Testing if the function is compatible
    if (state_fail(hsmp_send(0, HSMP_GET_DDR_BANDWIDTH, args, reps))) {
        debug("hsmp_send failed: %s", state_msg);
        close_all();
        return;
    }
    // ZEN3
    topology_select(tp, &tp_own, TPSelect.socket, TPGroup.merge, 0);
    line_size = (double) tp_own.cache_line_size;
    // Old init
    pool = calloc(tp_own.cpu_count + 1, sizeof(bwidth_t));
    timestamp_get(&pool[tp_own.cpu_count].time);
    // Pool suscription
    sus             = suscription();
    sus->call_init  = NULL;
    sus->call_main  = multipool;
    sus->time_relax = 5000;
    sus->time_burst = 5000;
    sus->suscribe(sus);
    //
    apis_put(ops->unload  , bwidth_amd19_unload);
    apis_put(ops->get_info, bwidth_amd19_get_info);
    apis_put(ops->read    , bwidth_amd19_read);
    debug("Loaded AMD19");
}

BWIDTH_F_UNLOAD(amd19)
{
    close_all();
}

BWIDTH_F_GET_INFO(amd19)
{
    info->api         = API_AMD19;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET;
    info->devs_count  = tp_own.cpu_count + 1;
}

BWIDTH_F_READ(amd19)
{
    // Update pool
    multipool(NULL);
    while (pthread_mutex_trylock(&lock));
    memcpy(b, pool, sizeof(bwidth_t) * (tp_own.cpu_count + 1));
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}
