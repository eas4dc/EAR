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
#include <common/system/time.h>
#include <common/output/debug.h>
#include <common/system/popen.h>
#include <common/system/monitor.h>
#include <common/math_operations.h>

typedef struct consumption_s {
    uint64_t    energy;        // mJ
    uint64_t    power_current; // mW
    timestamp_t timestamp; // We use our timestamp because DCMI fails
    uint64_t    samples;
} consumption_t;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint            devs_count;
static suscription_t  *sus;
static consumption_t  *pool;
static uint            opt;
static uint            is_power;
static char            cmd[4096];
static popen_t         script;

static state_t energy_pool(void *data);

state_t energy_init(void **x)
{
    state_t s = EAR_SUCCESS;
    static char host[128];
    char *hint;

    while (pthread_mutex_trylock(&lock));
    if (devs_count) {
        goto leave;
    }
    gethostname(host, sizeof(host));
    if (strchr(host, '.') != NULL) {
        *strchr(host, '.') = '\0';
    }
    // Detecting sensors depending on hardware
    if ((hint = getenv("EAR_ENERGY_ARGS")) != NULL) {
        switch ((opt = atoi(hint))) {
            case 1: // e4
                sprintf(cmd, "/opt/share/scripts/powerdiscovery/dcpower %s 63072000", host);
                is_power = 1;
                break;
        }
        debug("EAR_ENERGY_ARGS: %s", hint);
    }
    if (opt == 0) {
        pthread_mutex_unlock(&lock);
        return_msg(EAR_ERROR, "No sensors found.");
    }
    if (state_fail(s = popen_open(cmd, 1, 0, &script))) {
        return_msg(s, state_msg);
    }
    // Other initializations
    devs_count = 1;
    pool       = calloc(devs_count, sizeof(consumption_t));
    // Monitoring
    if (is_power) {
        sus             = suscription();
        sus->call_main  = energy_pool;
        sus->time_relax = 2000;
        sus->time_burst = 2000;
    }
leave:
    pthread_mutex_unlock(&lock);
    return s;
}

state_t energy_dispose(void **x)
{
    while (pthread_mutex_trylock(&lock));
    if (script.opened) {
        popen_close(&script);
    }
    monitor_unregister(sus);
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

static state_t energy_read(uint d, consumption_t *rd)
{
    int value;
    memset(rd, 0, sizeof(consumption_t));
    switch(opt) {
        case 1: // e4
            timestamp_get(&rd->timestamp);
            while(popen_read2(&script, ',', "aia", &value)) {
                rd->power_current += ((uint64_t) value) * 1000;
                rd->samples += 1;
            }
            if (rd->samples == 0) {
                return_msg(EAR_ERROR, "error during script execution");
            }
            rd->power_current = rd->power_current / rd->samples;
            rd->samples = 1;
            break;
    }
    debug("read->energy: %lu mJ", rd[d].energy);
    debug("read->power : %lu", rd[d].power_current);
    return EAR_SUCCESS;
}

#define goto_state(where, state) { state; goto where; }
#define goto_msg(where, state, message) { state; state_msg = message; goto where; }

static state_t energy_pool(void *data)
{
    state_t s        = EAR_SUCCESS;
    consumption_t rd = {0};
    double fpower;
    double ftime;
    int d;

    while (pthread_mutex_trylock(&lock));
    for (d = 0; d < devs_count; ++d) {
        // Cleaning and reading
        memset(&rd, 0, sizeof(consumption_t));
        if (state_fail(s = energy_read(d, &rd))) {
            goto_state(leave, s = EAR_ERROR);
        }
        // Computing time between samples
        ftime  = (double) timestamp_fdiff(&rd.timestamp, &pool[d].timestamp, TIME_SECS, TIME_MSECS);
        fpower = (pool[d].samples > 0) ? ((double) rd.power_current) * ftime : (double) rd.power_current;
        // If there are no changes between timestamps, continue
        if (ftime < 0.1) {
            goto_msg(leave, s = EAR_WARNING, "Insufficient time to get new samples");
        }
        // Pooling things
        pool[d].energy        = (is_power) ? pool[d].energy + (uint64_t) fpower : rd.energy;
        pool[d].power_current = rd.power_current;
        pool[d].timestamp     = rd.timestamp;
        pool[d].samples      += rd.samples;
        debug("pool->energy: %lu mJ", pool[d].energy);
        debug("pool->power : %lu mW (cur/min/max/avg)", pool[d].power_current);
        debug("pool->tstamp: %ld s, %ld ns", pool[d].timestamp.tv_sec, pool[d].timestamp.tv_nsec);
        debug("pool->fpower: %.02lf W", fpower);
        debug("pool->ftime : %.02lf s", ftime);
    }
leave:
    if (data != NULL) {
        memcpy(data, pool, sizeof(consumption_t) * devs_count);
    }
    pthread_mutex_unlock(&lock);
    return s;
}

state_t energy_dc_read(void *x, void *data)
{
    return energy_pool(data);
}

state_t energy_datasize(size_t *size)
{
    *size = sizeof(consumption_t) * devs_count;
    return EAR_SUCCESS;
}

state_t energy_units(uint *units)
{
    *units = 1000; // mJ
    return EAR_SUCCESS;
}

state_t energy_accumulated(ulong *energy_mj, void *data1, void *data2)
{
    consumption_t *readings2 = (consumption_t *) data2;
    consumption_t *readings1 = (consumption_t *) data1;

    *energy_mj = 0LU;
    if (readings2->samples != 0LLU && readings1->samples != 0LLU) {
        *energy_mj = overflow_zeros_u64(readings2->energy, readings1->energy);
#if SHOW_DEBUGS
        double time_s = (double) timestamp_fdiff(&readings2->timestamp, &readings1->timestamp, TIME_SECS, TIME_MSECS);
        debug("diff->energy: %lu mJ (%lu - %lu)", *energy_mj, readings2->energy, readings1->energy);
        debug("diff->fpower: %.02lf W", ((double) *energy_mj) / (time_s * 1000.0));
        debug("diff->ftime : %.02lf s", time_s);
#endif
    }
    return EAR_SUCCESS;
}

state_t energy_to_str(char *buffer, void *data)
{
    sprintf(buffer, "%lu", ((consumption_t *) data)->energy);
    return EAR_SUCCESS;
}

uint energy_data_is_null(void *data)
{
    return ((consumption_t *) data)->energy == 0;
}

state_t energy_frequency(ulong *freq_us)
{
    *freq_us = 10000;
    return EAR_SUCCESS;
}