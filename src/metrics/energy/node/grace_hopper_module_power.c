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
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/hardware/hardware_info.h>
#include <common/math_operations.h>
#include <common/output/debug.h>
#include <common/sizes.h>
#include <common/system/monitor.h>
#include <metrics/common/apis.h>
#include <metrics/common/hwmon.h>

static pthread_mutex_t lock  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t elock = PTHREAD_MUTEX_INITIALIZER;
static uint socket_count     = 0;

#define GH_MODULE_PERIOD 3000
#define NUM_DEVICES      1
#define VGRACE_VL        2
static suscription_t *gh_module_power_sus;
static state_t gh_module_power_mon_main(void *p);
static state_t gh_module_power_mon_init(void *p);
static state_t gh_module_static_read(ctx_t *c, llong *power, llong *acum, state_t *st_read);
static uint gh_module_power_initialized = 0;
static llong *aux_power;
static llong aux_acum_energy = 0;

typedef struct hwfds_s {
    uint id_count;
    uint *fd_count;
    int **fds;
} hwfds_t;

static ctx_t local_ctx;
static ctx_t *plocal_ctx = NULL;

static state_t gh_module_power_mon_init(void *p)
{
    verbose(VGRACE_VL, "GH Module: monitor init");
    if (!gh_module_power_initialized)
        return EAR_ERROR;
    return EAR_SUCCESS;
}

static uint accumulated_periods = 1;

static state_t gh_module_power_mon_main(void *p)
{
    state_t st_read;
    if (!gh_module_power_initialized)
        return EAR_ERROR;
    llong acum;

    pthread_mutex_lock(&lock);

    gh_module_static_read(plocal_ctx, aux_power, &acum, &st_read);
    /* Assuming GH_MODULE_PERIOD elapsed time */
    /* PERIOD is in msec. acum is in uWatts */
    if (acum) {
        aux_acum_energy += acum * GH_MODULE_PERIOD * accumulated_periods / 1000000;
        accumulated_periods = 1;
    } else {
        // If we uncomment this line, we will accumulate the power since the last reading
        // this strategy results in too much variability

        // accumulated_periods++;
    }

    pthread_mutex_unlock(&lock);

    verbose(VGRACE_VL, "GH Module: Adding %lld W aux_acum_energy %lld J (OK %u) (periods %u)\n", acum / 1000000,
            aux_acum_energy / 1000, state_ok(st_read), accumulated_periods);

    return EAR_SUCCESS;
}

static state_t gh_module_power_load(topology_t *topo)
{
    state_t s;
    debug("asking for status");
    while (pthread_mutex_trylock(&lock)) {
    };
    if (socket_count == 0) {
        if (state_ok(s = hwmon_find_drivers("Module Power Socket 0", NULL, NULL))) {
            socket_count = topo->socket_count;
        }
    }
    debug("gh_module: %d sockets found", socket_count);
    pthread_mutex_unlock(&lock);
    /* We should try to open the file for reading */
    return s;
}

static state_t gh_module_power_init(ctx_t *c)
{
    uint id_count;
    uint *ids;
    state_t s;
    int i;

    pthread_mutex_lock(&lock);
    if (gh_module_power_initialized) {
        pthread_mutex_unlock(&lock);
        return EAR_SUCCESS;
    }

    debug("asking for init");
    // Looking for ids
    verbose(VGRACE_VL, "GH Module: testing 'Module Power Socket 0' drivers");
    if (xtate_fail(s, hwmon_find_drivers("Module Power Socket 0", &ids, &id_count))) {
        verbose(VGRACE_VL, "GH Module: drivers search failed");
        pthread_mutex_unlock(&lock);
        return s;
    }
    verbose(VGRACE_VL, "GH Module: %d devices found", id_count);
    if (id_count == 0) {
        pthread_mutex_unlock(&lock);
        return EAR_ERROR;
    }
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
    verbose(VGRACE_VL, "GH Module: opening devices");
    for (i = 0; i < h->id_count; ++i) {
        if (xtate_fail(s, hwmon_open_files(ids[i], Hwmon_power_meter.power_avg, &h->fds[i], &h->fd_count[i], 1))) {
            verbose(VGRACE_VL, "GH Module: opening fails");
            pthread_mutex_unlock(&lock);
            return s;
        }
    }
    // Freeing ids space
    free(ids);
    debug("init succeded");
    gh_module_power_initialized = 1;

    /* Lock is freed before the possible monitor creation, so we avoid creating a thread with
     * the lock holded. Moreover, since gh_module_power_initialized is set to 1, any other thread
     * will enter to this routine any more, so it is save to unlock the mutex. */
    pthread_mutex_unlock(&lock);

    verbose(VGRACE_VL, "GH Module: starting monitor");

    /* Power must be accumulated */
    debug("Creating suscrition ");
    if (!monitor_is_initialized())
        monitor_init();
    gh_module_power_sus             = suscription();
    gh_module_power_sus->call_main  = gh_module_power_mon_main;
    gh_module_power_sus->call_init  = gh_module_power_mon_init;
    gh_module_power_sus->time_relax = GH_MODULE_PERIOD;
    gh_module_power_sus->time_burst = GH_MODULE_PERIOD;
    gh_module_power_sus->suscribe(gh_module_power_sus);
    debug("Suscription done");

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

static state_t gh_module_power_read(ctx_t *c, ullong *values)
{
    if (values == NULL)
        return EAR_ERROR;
    if (!gh_module_power_initialized)
        return EAR_ERROR;
    /* Energy CPU assumes N uncore values (1 per socket) and N core values (1 per socket) */
    /* Units are nano Joules */
    memset(values, 0, sizeof(ullong) * socket_count * NUM_DEVICES);
    /* In this API energy is reported in uJoules */
    /* We don't know yet how to detect different sockets so socket 1 is used */
    /* We use socket_count because this API returns DRAM and PCK */
    values[socket_count] = (ullong) aux_acum_energy;
    return EAR_SUCCESS;
}

static state_t gh_module_power_dispose(ctx_t *c)
{
    int i;
    hwfds_t *h;
    state_t s;

    if (!gh_module_power_initialized)
        return EAR_ERROR;
    return EAR_SUCCESS;

    /* WARNING: Below code is never reached. */

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
static state_t gh_module_power_count_devices(ctx_t *c, uint *count)
{
    hwfds_t *h;
    state_t s;

    debug("gh_module_power_count_devices");

    if (xtate_fail(s, get_context(c, &h))) {
        return s;
    }
    if ((count != NULL) && (h != NULL)) {
        //*count = h->id_count;
        // To be compatible with other APIs we will use
        // socket granularity for now
        *count = socket_count;
    }

    return EAR_SUCCESS;
}

static state_t gh_module_static_read(ctx_t *c, llong *power, llong *acum, state_t *st_read)
{
    char data[SZ_PATH];
    llong aux1, aux2 = 0;
    int i, k;
    hwfds_t *h;
    state_t s;

    *st_read = EAR_SUCCESS;

    if (xtate_fail(s, get_context(c, &h))) {
        *st_read = s;
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
                *st_read = s;
                debug("read failed with data %s", data);
            } else {
                //	debug("read %s", data);
                aux1 = atoll(data);
                verbose(VGRACE_VL, "GH Module: Partial power read %lld", aux1);
                power[i] += aux1;
                aux2 += aux1;
            }
        }
    }
    //
    if (acum != NULL)
        *acum = aux2;

    return EAR_SUCCESS;
}

/*
 * ENERGY NODE API
 */
#include <common/system/time.h>
#include <metrics/energy/node/energy_node.h>

static timestamp_t start_time;
static uint init;
static topology_t tp;
static ctx_t gh_ctx;
static uint grace_module_energy_init = 0;

/*
 * MAIN FUNCTIONS
 */

static state_t static_energy_init()
{
    state_t st = EAR_SUCCESS;

    while (pthread_mutex_trylock(&elock)) {
    };
    if (grace_module_energy_init) {
        pthread_mutex_unlock(&elock);
        return st;
    }
    topology_init(&tp);
    verbose(VGRACE_VL, "GH Module: init");
    if (gh_module_power_load(&tp) != EAR_SUCCESS) {
        pthread_mutex_unlock(&elock);
        return EAR_ERROR;
    }
    if (state_ok(st = gh_module_power_init(&gh_ctx)))
        grace_module_energy_init = 1;
    pthread_mutex_unlock(&elock);
    return st;
}

state_t energy_init(void **c)
{
    return static_energy_init();
}

state_t energy_dispose(void **c)
{
    verbose(VGRACE_VL, "GH Module: dispose");
    if (grace_module_energy_init)
        gh_module_power_dispose(&gh_ctx);
    return EAR_SUCCESS;
}

state_t energy_datasize(size_t *size)
{
    verbose(VGRACE_VL, "GH Module: datasize");
    debug("energy_datasize %lu\n", sizeof(unsigned long));
    *size = sizeof(unsigned long);
    return EAR_SUCCESS;
}

state_t energy_frequency(ulong *freq_us)
{
    verbose(VGRACE_VL, "GH Module: freq");
    *freq_us = 1000000;
    return EAR_SUCCESS;
}

state_t energy_to_str(char *str, edata_t e)
{
    ulong *pe = (ulong *) e;
    sprintf(str, "%lu", *pe);
    return EAR_SUCCESS;
}

static unsigned long gh_diff_node_energy(ulong init, ulong end)
{
    ulong ret = 0;
    if (end > init) {
        ret = end - init;
    } else {
        ret = ulong_diff_overflow(init, end);
    }
    return ret;
}

state_t energy_units(uint *units)
{
    verbose(VGRACE_VL, "GH Module: units");
    *units = 1000;
    return EAR_SUCCESS;
}

state_t energy_accumulated(unsigned long *e, edata_t init, edata_t end)
{

    verbose(VGRACE_VL, "GH Module: energy accumulated");
    ulong *pinit = (ulong *) init, *pend = (ulong *) end;

    ulong total = gh_diff_node_energy(*pinit, *pend);
    *e          = total;
    return EAR_SUCCESS;
}

/* These two functions are pending to use a lock and update the energy since the last reading */
state_t energy_dc_read(void *c, edata_t energy_mj)
{
    verbose(VGRACE_VL, "GH Module: energy read");
    if (!grace_module_energy_init)
        return EAR_ERROR;
    ulong *penergy_mj = (ulong *) energy_mj;

    *penergy_mj = (ulong) aux_acum_energy;

    return EAR_SUCCESS;
}

state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms)
{
    verbose(VGRACE_VL, "GH Module: energy & time read");
    if (!grace_module_energy_init)
        return EAR_ERROR;
    ulong *penergy_mj = (ulong *) energy_mj;

    ullong elapsed = timestamp_diffnow(&start_time, TIME_MSECS);

    *penergy_mj = (ulong) aux_acum_energy;
    debug("energy_dc_read: %lu elapsed: %llu\n", *penergy_mj, elapsed);
    *time_ms = (ulong) time(NULL) * 1000;

    return EAR_SUCCESS;
}

uint energy_data_is_null(edata_t e)
{
    verbose(VGRACE_VL, "GH Module: data is null");
    ulong *pe = (ulong *) e;
    return (*pe == 0);
}

state_t energy_not_privileged_init()
{
    return static_energy_init();
}

uint energy_is_privileged()
{
    // This function needs more robustness.
    // We should call the static read and check state
    return 0;
}
