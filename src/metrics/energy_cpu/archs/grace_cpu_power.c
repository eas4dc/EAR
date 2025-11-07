/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

#define SHOW_DEBUGS 1
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/hardware/hardware_info.h>
#include <common/output/debug.h>
#include <common/sizes.h>
#include <common/system/monitor.h>
#include <metrics/common/apis.h>
#include <metrics/common/hwmon.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint socket_count    = 0;

#define GH_CPU_PERIOD 2000
#define NUM_DEVICES   2 // DRAM and PKG
#define VGRACE_CPU_VL 3
static suscription_t *grace_cpu_sus;
static state_t grace_cpu_mon_main(void *p);
static state_t grace_cpu_mon_init(void *p);
static state_t grace_cpu_static_read(ctx_t *c, llong *power, llong *acum);
static uint grace_cpu_initialized = 0;
static llong *aux_power;
static llong aux_acum_energy = 0;

typedef struct hwfds_s {
    uint id_count;
    uint *fd_count;
    int **fds;
} hwfds_t;

static ctx_t local_ctx;
static ctx_t *plocal_ctx = NULL;

static state_t grace_cpu_mon_init(void *p)
{
    if (!grace_cpu_initialized)
        return EAR_ERROR;
    return EAR_SUCCESS;
}

static uint accumulated_periods = 1;

static state_t grace_cpu_mon_main(void *p)
{
    if (!grace_cpu_initialized)
        return EAR_ERROR;
    llong acum;
    grace_cpu_static_read(plocal_ctx, aux_power, &acum);
    /* Assuming GH_CPU_PERIOD elapsed time */
    /* PERIOD is in msec. acum is in uWatts */
    if (acum) {
        pthread_mutex_lock(&lock);
        aux_acum_energy += acum * GH_CPU_PERIOD * accumulated_periods;
        accumulated_periods = 1;
        pthread_mutex_unlock(&lock);
    } else {
        // If we uncomment this line, we will accumulate the power since the last reading
        // this strategy results in too much variability
        // pthread_mutex_lock(&lock);
        // accumulated_periods++;
        // pthread_mutex_unlock(&lock);
    }

    verbose(VGRACE_CPU_VL, "GH CPU: Adding %lld W aux_acum_energy %lld J (periods %u)\n", acum / 1000000,
            aux_acum_energy / 1000000000, accumulated_periods);

    return EAR_SUCCESS;
}

state_t grace_cpu_load(topology_t *topo)
{
    state_t s;
    debug("asking for status");
    while (pthread_mutex_trylock(&lock))
        ;
    if (socket_count == 0) {
        if (state_ok(s = hwmon_find_drivers("Grace Power Socket", NULL, NULL))) {
            socket_count = topo->socket_count;
        }
    }
    verbose(VGRACE_CPU_VL, "grace_cpu: %d sockets found", socket_count);
    pthread_mutex_unlock(&lock);
    /* We should try to open the file for reading */
    return s;
}

state_t grace_cpu_init(ctx_t *c)
{
    uint id_count;
    uint *ids;
    state_t s;
    int i;

    verbose(VGRACE_CPU_VL, "GH CPU: Init ");

    pthread_mutex_lock(&lock);
    if (grace_cpu_initialized) {
        pthread_mutex_unlock(&lock);
        return EAR_SUCCESS;
    }

    debug("asking for init");
    // Looking for ids
    debug("grace_cpu_init looking for devices");
    if (xtate_fail(s, hwmon_find_drivers("Grace Power Socket", &ids, &id_count))) {
        pthread_mutex_unlock(&lock);
        return s;
    }
    verbose(VGRACE_CPU_VL, "GH CPU: %d devices found", id_count);
    if (id_count == 0) {
        pthread_mutex_unlock(&lock);
        return EAR_ERROR;
    }
    debug("%d devices found", id_count);
    // Allocating context
    if (c == no_ctx) {
        plocal_ctx = &local_ctx;
    } else {
        plocal_ctx = c;
    }
    if ((plocal_ctx->context = calloc(1, sizeof(hwfds_t))) == NULL) {
        pthread_mutex_unlock(&lock);
        return_msg(s, strerror(errno));
    }
    // Allocating file descriptors)
    hwfds_t *h = (hwfds_t *) plocal_ctx->context;
    /* fd is a per device a per num sockets vector */
    if ((h->fds = calloc(id_count, sizeof(int *))) == NULL) {
        pthread_mutex_unlock(&lock);
        return_msg(s, strerror(errno));
    }
    if ((h->fd_count = calloc(id_count, sizeof(uint))) == NULL) {
        pthread_mutex_unlock(&lock);
        return_msg(s, strerror(errno));
    }
    h->id_count = id_count;
    aux_power   = calloc(id_count, sizeof(llong));
    if (aux_power == NULL) {
        pthread_mutex_unlock(&lock);
        return EAR_ERROR;
    }
    //
    for (i = 0; i < h->id_count; ++i) {
        if (xtate_fail(s, hwmon_open_files(ids[i], Hwmon_power_meter.power_avg, &h->fds[i], &h->fd_count[i], 1))) {
            pthread_mutex_unlock(&lock);
            return s;
        }
    }
    // Freeing ids space
    free(ids);
    debug("init succeded");
    grace_cpu_initialized = 1;

    /* Power must be accumulated */

    // Release the lock just before (occasionally) creating another thread
    // As we already set grace_cpu_initialized to non-zero, we know any other
    // thread won't enter again, so monitor stuff don't need to lock
    pthread_mutex_unlock(&lock);

    debug("Creating suscrition ");
    if (!monitor_is_initialized())
        monitor_init();

    grace_cpu_sus             = suscription();
    grace_cpu_sus->call_main  = grace_cpu_mon_main;
    grace_cpu_sus->call_init  = grace_cpu_mon_init;
    grace_cpu_sus->time_relax = GH_CPU_PERIOD;
    grace_cpu_sus->time_burst = GH_CPU_PERIOD;

    grace_cpu_sus->suscribe(grace_cpu_sus);
    debug("Suscription done");

    verbose(VGRACE_CPU_VL, "GH CPU:  Init done");
    return EAR_SUCCESS;
}

static state_t get_context(ctx_t *c, hwfds_t **h)
{
    ctx_t *curr;
    if (c == no_ctx) {
        curr = &local_ctx;
    } else {
        curr = c;
    }

    *h = (hwfds_t *) curr->context;
    return EAR_SUCCESS;
}

state_t grace_cpu_read(ctx_t *c, ullong *values)
{
    if (values == NULL)
        return EAR_ERROR;
    if (!grace_cpu_initialized)
        return EAR_ERROR;
    /* Energy CPU assumes N uncore values (1 per socket) and N core values (1 per socket) */
    /* Units are nano Joules */
    memset(values, 0, sizeof(ullong) * socket_count * NUM_DEVICES);
    /* In this API energy is reported in uJoules */
    /* We don't know yet how to detect different sockets so socket 1 is used */
    /* We use socket_count because this API returns DRAM and PCK */
    verbose(VGRACE_CPU_VL, "GH CPU: energy %llu", (ullong) aux_acum_energy);
    // S0 [DRAM, PKG] S1 [DRAM, PKG] etc
    values[socket_count] = (ullong) aux_acum_energy;
    return EAR_SUCCESS;
}

state_t grace_cpu_dispose(ctx_t *c)
{
    int i;
    hwfds_t *h;
    state_t s;

    if (!grace_cpu_initialized)
        return EAR_ERROR;

    if (xtate_fail(s, get_context(c, &h))) {
        return s;
    }
    for (i = 0; i < h->id_count; ++i) {
        hwmon_close_files(h->fds[i], h->fd_count[i]);
    }
    if (c != NULL)
        c->context = NULL;
    free(h->fd_count);
    free(h->fds);

    return EAR_SUCCESS;
}

// Data
state_t grace_cpu_count_devices(ctx_t *c, uint *count)
{
    hwfds_t *h;
    state_t s;

    // TODO: socket_count variable is available once the API is loaded,
    // so technically a user could call this function before calling grace_cpu_init

    verbose(VGRACE_CPU_VL, "GH CPU: count devices %u", socket_count);

    if (count != NULL) {
        //*count = h->id_count;
        // To be compatible with other APIs we will use
        // socket granularity for now
        *count = socket_count;
    }

    return EAR_SUCCESS;
}

static state_t grace_cpu_static_read(ctx_t *c, llong *power, llong *acum)
{
    char data[SZ_PATH];
    llong aux1, aux2 = 0;
    int i, k;
    hwfds_t *h;
    state_t s;

    if (xtate_fail(s, get_context(c, &h))) {
        return s;
    }
    if (power == NULL)
        return EAR_ERROR;
    memset(power, 0, sizeof(llong) * h->id_count);
    if (acum != NULL)
        *acum = 0;
    for (i = 0; i < h->id_count; ++i) {
        // debug("I %d has %d Fds", i, h->fd_count[i]);
        for (k = 0; k < h->fd_count[i]; k++) {
            memset(data, 0, sizeof(data));
            //	debug("Reading ID %d,%d  fd=%d", i, k, h->fds[i][k]);

            if (xtate_fail(s, hwmon_read_files(h->fds[i][k], data, SZ_PATH))) {
                // return s;
                debug("read failed with data %s", data);
            }
            debug("read %s", data);
            aux1 = atoll(data);
            verbose(VGRACE_CPU_VL, "GH CPU: Partial power read %lld", aux1);
            power[i] += aux1;
            aux2 += aux1;
        }
    }
    //
    if (acum != NULL)
        *acum = aux2;

    return EAR_SUCCESS;
}
