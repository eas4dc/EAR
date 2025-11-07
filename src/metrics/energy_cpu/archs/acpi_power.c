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

#define ACPI_POWER_PERIOD 3000
#define NUM_DEVICES       2
static suscription_t *acpi_power_meter_sus;
static state_t acpi_power_meter_mon_main(void *p);
static state_t acpi_power_meter_mon_init(void *p);
static state_t acpi_power_static_read(ctx_t *c, llong *power, llong *acum);
static uint acpi_power_meter_initialized = 0;
static llong *aux_power;
static llong aux_acum_energy = 0;
static state_t init_st;

typedef struct hwfds_s {
    uint id_count;
    uint *fd_count;
    int **fds;
} hwfds_t;

static ctx_t local_ctx;
static ctx_t *plocal_ctx = NULL;
static state_t static_init(ctx_t *c);

static state_t acpi_power_meter_mon_init(void *p)
{
    if (!acpi_power_meter_initialized)
        return EAR_ERROR;
    return EAR_SUCCESS;
}

static state_t acpi_power_meter_mon_main(void *p)
{
    if (!acpi_power_meter_initialized)
        return EAR_ERROR;
    llong acum;
    acpi_power_static_read(plocal_ctx, aux_power, &acum);
    /* Assuming ACPI_POWER_PERIOD elapsed time */
    /* PERIOD is in msec. acum is in uWatts */
    aux_acum_energy += acum * ACPI_POWER_PERIOD;

    debug("Adding %lld W aux_acum_energy %lld J\n", acum / 1000000, aux_acum_energy);

    return EAR_SUCCESS;
}

state_t acpi_power_meter_load(topology_t *topo)
{
    state_t s;
    debug("asking for status");
    while (pthread_mutex_trylock(&lock))
        ;
    if (socket_count == 0) {
        if (state_ok(s = hwmon_find_drivers("power_meter", NULL, NULL))) {
            socket_count = topo->socket_count;
        }
    }
    debug("acpi_power: %d sockets found", socket_count);
    pthread_mutex_unlock(&lock);
    /* We should try to open the file for reading */
    init_st = static_init(no_ctx);
    return init_st;
}

static state_t static_init(ctx_t *c)
{
    uint id_count;
    uint *ids;
    state_t s;
    int i;

    pthread_mutex_lock(&lock);
    if (acpi_power_meter_initialized) {
        pthread_mutex_unlock(&lock);
        return EAR_SUCCESS;
    }

    debug("asking for init");
    // Looking for ids
    debug("acpi_power_init looking for devices");
    if (xtate_fail(s, hwmon_find_drivers("power_meter", &ids, &id_count))) {
        pthread_mutex_unlock(&lock);
        return s;
    }
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
    acpi_power_meter_initialized = 1;

    /* Power must be accumulated */
    debug("Creating suscrition ");
    if (!monitor_is_initialized())
        monitor_init();
    acpi_power_meter_sus             = suscription();
    acpi_power_meter_sus->call_main  = acpi_power_meter_mon_main;
    acpi_power_meter_sus->call_init  = acpi_power_meter_mon_init;
    acpi_power_meter_sus->time_relax = ACPI_POWER_PERIOD;
    acpi_power_meter_sus->time_burst = ACPI_POWER_PERIOD;
    acpi_power_meter_sus->suscribe(acpi_power_meter_sus);
    debug("Suscription done");

    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t acpi_power_meter_init(ctx_t *c)
{
    if (acpi_power_meter_initialized)
        return init_st;
    init_st = static_init(c);
    return init_st;
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

state_t acpi_power_meter_read(ctx_t *c, ullong *values)
{
    if (values == NULL)
        return EAR_ERROR;
    if (!acpi_power_meter_initialized)
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

state_t acpi_power_meter_dispose(ctx_t *c)
{
    int i;
    hwfds_t *h;
    state_t s;

    if (!acpi_power_meter_initialized)
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
state_t acpi_power_meter_count_devices(ctx_t *c, uint *count)
{
    hwfds_t *h;
    state_t s;

    debug("acpi_power_meter_count_devices");

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

static state_t acpi_power_static_read(ctx_t *c, llong *power, llong *acum)
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
        debug("I %d has %d Fds", i, h->fd_count[i]);
        for (k = 0; k < h->fd_count[i]; k++) {
            memset(data, 0, sizeof(data));
            debug("Reading ID %d,%d  fd=%d", i, k, h->fds[i][k]);

            if (xtate_fail(s, hwmon_read_files(h->fds[i][k], data, SZ_PATH))) {
                // return s;
                debug("read failed with data %s", data);
            }
            debug("aux2 %lld", aux2);
            debug("read %s", data);
            aux1 = atoll(data);
            debug("aux1 %lld", aux1);
            power[i] += aux1;
            aux2 += aux1;
            debug("power[%d] %lld", i, power[i]);
            debug("aux2 %lld", aux2);
        }
    }
    //
    if (acum != NULL)
        *acum = aux2;

    return EAR_SUCCESS;
}

#if TEST
// gcc -o test -DTEST -I$HOME/POWER_METER_TEST/ear_private/src acpi_power.c ../../../common/libcommon.a
// ../../libmetrics.a
int main(int argc, char *argv[])
{
    topology_t my_topology;
    topology_init(&my_topology);
    if (state_fail(acpi_power_meter_load(&my_topology))) {
        printf("Error in load\n");
        exit(1);
    }
    printf("ACPI power meter loaded\n");
    ctx_t c;
    if (state_fail(acpi_power_meter_init(&c))) {
        printf("Error in init\n");
        exit(1);
    }
    printf("ACPI power meter initialized\n");
    uint devices;
    if (state_fail(acpi_power_count_devices(&c, &devices))) {
        printf("Error in count devices\n");
        exit(1);
    }
    printf("ACPI power meter %u devices detected\n", devices);
    ullong *power_socket;
    ullong *last_power_socket;
    ullong *diff;
    power_socket      = calloc(devices * NUM_DEVICES, sizeof(ullong));
    last_power_socket = calloc(devices * NUM_DEVICES, sizeof(ullong));
    diff              = calloc(devices * NUM_DEVICES, sizeof(ullong));
    do {
        acpi_power_meter_read(&c, power_socket);
        for (uint i = 0; i < devices * NUM_DEVICES; i++) {
            diff[i] = power_socket[i] - last_power_socket[i];
            printf("ACPI power [%u] = %.2lfW\n", i, (double) diff[i] / (double) (10 * 1000000000.0));
            last_power_socket[i] = power_socket[i];
        }
        sleep(10);
    } while (1);
    acpi_power_meter_dispose(&c);
    return 0;
}
#endif
