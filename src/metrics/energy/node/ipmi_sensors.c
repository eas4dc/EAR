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
#include <common/system/monitor.h>
#include <common/system/time.h>
#include <metrics/common/ipmi.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct consumption_s {
    uint64_t energy;       // mJ
    timestamp_t timestamp; // We use our timestamp because DCMI fails
    uint64_t samples;
} consumption_t;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static ipmi_sensor_t *sensors;
static uint sensors_count;
static ipmi_reading_t *sensors_readings;
static suscription_t *sus;
static consumption_t *pool;
static uint opt_sysp[11] = {1, 1, 1000}; // Sys Power (power)
static uint opt_lm50[11] = {2, 1, 1000}; // LM5066_PIN (power)
static uint *opt         = NULL;

static state_t energy_pool(void *data);

state_t energy_init(void **x)
{
    state_t s = EAR_SUCCESS;
    while (pthread_mutex_trylock(&lock))
        ;
    if (sensors_count) {
        goto leave;
    }
    if (state_fail(s = ipmi_open())) {
        goto leave;
    }
    // Detecting sensors depending on hardware
    if (ipmi_sensors_find("LM5066_PIN,CWG_LM5066I_PIN", &sensors, &sensors_count)) {
        opt = opt_lm50;
    } else if (ipmi_sensors_find("Sys Power", &sensors, &sensors_count)) {
        opt = opt_sysp;
    } else if (ipmi_sensors_find("SYS_POWER", &sensors, &sensors_count)) {
        opt = opt_sysp;
    }
    // If no sensors, error
    if (sensors_count == 0) {
        pthread_mutex_unlock(&lock);
        return_msg(EAR_ERROR, "No sensors found.");
    }
    pool = calloc(sensors_count, sizeof(consumption_t));
    // Monitoring
    if (opt[1]) {
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
    return EAR_SUCCESS;
}

#define goto_state(where, state)                                                                                       \
    {                                                                                                                  \
        state;                                                                                                         \
        goto where;                                                                                                    \
    }

#define goto_msg(where, state, message)                                                                                \
    {                                                                                                                  \
        state;                                                                                                         \
        state_msg = message;                                                                                           \
        goto where;                                                                                                    \
    }

static state_t energy_pool(void *data)
{
    state_t st     = EAR_SUCCESS;
    timestamp_t ts = {0};
    double fpower;
    double ftime;
    int s;

    while (pthread_mutex_trylock(&lock));
    //
    if (state_fail(st = ipmi_sensors_read(sensors, sensors_count, &sensors_readings))) {
        goto_state(leave, st = EAR_ERROR);
    }
    timestamp_get(&ts);
    // If there are no changes between timestamps, continue
    for (s = 0; s < sensors_count; ++s) {
        // Computing time between samples
        ftime  = (double) timestamp_fdiff(&ts, &pool[s].timestamp, TIME_SECS, TIME_MSECS);
        fpower = (pool[s].samples > 0) ? sensors_readings[s].value * ftime : sensors_readings[s].value;
        fpower *= (double) opt[2];

        debug("pool->ftime : %.02lf s", ftime);
        debug("pool->fpower: %0.02lf W", fpower);
        if (ftime < 0.1 && pool[s].samples > 0) {
            goto_msg(leave, st = EAR_WARNING, "Insufficient time to get new samples");
        }
        // Pooling things
        pool[s].energy    = (opt[1]) ? ((uint64_t) fpower) + pool[s].energy : ((uint64_t) sensors_readings[s].value);
        pool[s].timestamp = ts;
        pool[s].samples += 1;
        debug("pool->sensor: %0.02lf", sensors_readings[s].value);
        debug("pool->tstamp: %ld s, %ld ns", pool[s].timestamp.tv_sec, pool[s].timestamp.tv_nsec);
        debug("pool->energy: %lu mJ", pool[s].energy);
    }
leave:
    if (data != NULL) {
        memcpy(data, pool, sizeof(consumption_t) * sensors_count);
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
    *size = sizeof(consumption_t) * sensors_count;
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
    int s;

    *energy_mj = 0LU;
    for (s = 0; s < sensors_count; ++s) {
        if (readings2->samples != 0LLU && readings1->samples != 0LLU) {
            *energy_mj += overflow_zeros_u64(readings2[s].energy, readings1[s].energy);
#if SHOW_DEBUGS
            double time_s =
                (double) timestamp_fdiff(&readings2[s].timestamp, &readings1[s].timestamp, TIME_SECS, TIME_MSECS);
            debug("diff->energy: %lu mJ (%lu - %lu)", *energy_mj, readings2[s].energy, readings1[s].energy);
            debug("diff->fpower: %.02lf W", ((double) *energy_mj) / (time_s * 1000.0));
            debug("diff->ftime : %.02lf s (%ld - %ld s)", time_s, readings2[s].timestamp.tv_sec,
                  readings1[s].timestamp.tv_sec);
#endif
        }
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