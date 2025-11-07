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
// #define SHOW_DEBUGS_ULTRA 1

#include <common/math_operations.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/system/time.h>
#include <metrics/common/ipmi.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct consumption_s {
    uint64_t energy;        // mJ
    uint64_t power_current; // mW
    uint64_t power_minimum;
    uint64_t power_maximum;
    uint64_t power_average;
    uint64_t timeframe;
    timestamp_t timestamp; // We use our timestamp because DCMI fails
    uint64_t samples;
} consumption_t;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static uint devs_count;
static suscription_t *sus;
static consumption_t *pool;
static char opt_sd650e[11] = {1, 0, 0x3a, 0x32, 5, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00}; // Lenovo SD650 (energy)
static char opt_sd650p[11] = {2, 1, 0x3a, 0x32, 5, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00}; // Lenovo SD650 (power)
static char opt_dcmi[11]   = {3,    1,    0x2c, 0x02, 4,   0xdc,
                              0x01, 0x00, 0x00, 0x00, 0x00}; // Data Center Manageability Interface (power)
static char opt_inmp[11]   = {4, 1, 0x2e, 0xc8, 6, 0x57, 0x01, 0x00, 0x01, 0x00, 0x00}; // Intel Node Manager (power)
static char *opt           = NULL;

static state_t energy_pool(void *data);

static int send_oem_message(int d, ipmi_cmd_t *pkg)
{
    state_t s;
    // Addresses and messages are shared among all devices
    ipmi_cmd_set_addr(pkg, IPMI_SYSTEM_INTERFACE_ADDR_TYPE, IPMI_BMC_CHANNEL, 0x00, 0x00);
    ipmi_cmd_set_msg(pkg, opt[2], opt[3], opt[4]);
    memcpy(pkg->msg_send.data, &opt[5], 6);

    if (state_fail(s = ipmi_cmd_send(d, pkg))) {
        return 0;
    }
    if (pkg->msg_recv_data[0] != 0) {
        return_print(0, "invalid completion code (0x%x)", pkg->msg_recv_data[0]);
    }
#if SHOW_DEBUGS_ULTRA
    int i;
    for (i = 0; i < 16; ++i) {
        debug("data[%d]: 0x%x", i, pkg->msg_recv_data[i]);
    }
#endif
    return 1;
}

state_t energy_init(void **x)
{
    state_t s = EAR_SUCCESS;
    char *hint;

    while (pthread_mutex_trylock(&lock)) ;
    if (devs_count) {
        goto leave;
    }
    if (state_fail(s = ipmi_open())) {
        goto leave;
    }
    // Detecting sensors depending on hardware
    if ((hint = getenv("EAR_ENERGY_ARG")) != NULL) {
        switch (atoi(hint)) {
            case 1: opt = opt_sd650e; break;
            case 2: opt = opt_sd650p; break;
            case 3: opt = opt_dcmi;   break;
            case 4: opt = opt_inmp;   break;
        }
        debug("EAR_ENERGY_ARG: %s", hint);
    } else if (ipmi_has_hardware("SD650"))
        opt = opt_sd650e;
    else if (ipmi_has_hardware("ThinkSystem"))
        opt = opt_dcmi;
    // If no sensors, error
    if (opt == NULL) {
        pthread_mutex_unlock(&lock);
        return_msg(EAR_ERROR, "No sensors found.");
    }
    // Other initializations
    devs_count = ipmi_devs_count();
    pool       = calloc(devs_count, sizeof(consumption_t));
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

static void parse(char id, uchar *data, consumption_t *reading)
{
    // Getting timestamp
    // timestamp_get(&reading->timestamp);
    //
    switch (id) {
        case 1: // Lenovo SD650 (energy)
            reading->energy = ((uint64_t) *((uint32_t *) &data[3])) * 1000LLU + ((uint64_t) *((uint16_t *) &data[7]));
            reading->timestamp.tv_sec  = ((uint64_t) *((uint32_t *) &data[9]));
            reading->timestamp.tv_nsec = ((uint64_t) *((uint16_t *) &data[13])) * 1000000LLU;
            break;
        case 2: // Lenovo SD650 (power)
            // The energy_sd650_power.so plugin uses these indexes to get the timestamp, but
            // I found that weren't working and the correct timestamp uses the same indexes
            // than the energy_sd650.so. Then, by now we are going to use the same values.
            reading->timestamp.tv_sec  = ((uint64_t) *((uint32_t *) &data[1]));
            reading->timestamp.tv_nsec = ((uint64_t) *((uint16_t *) &data[5])) * 1000000LLU;
            reading->power_current =
                ((uint64_t) *((uint16_t *) &data[7])) * 1000LLU + ((uint64_t) *((uint16_t *) &data[9])) * 1000LLU;
            break;
        case 3: // Data Center Manageability Interface (DCMI)
            reading->power_current     = (uint64_t) *((uint16_t *) &data[2]) * 1000LLU;
            reading->power_minimum     = (uint64_t) *((uint16_t *) &data[4]) * 1000LLU;
            reading->power_maximum     = (uint64_t) *((uint16_t *) &data[6]) * 1000LLU;
            reading->power_average     = (uint64_t) *((uint16_t *) &data[8]) * 1000LLU;
            reading->timestamp.tv_sec  = (uint64_t) *((uint32_t *) &data[10]);
            reading->timestamp.tv_nsec = 0;
            reading->timeframe         = (uint64_t) *((uint32_t *) &data[14]);
            break;
        case 4: // Intel Node Manager (INM)
            reading->power_current     = (uint64_t) *((uint16_t *) &data[4]) * 1000LLU;
            reading->power_minimum     = (uint64_t) *((uint16_t *) &data[6]) * 1000LLU;
            reading->power_maximum     = (uint64_t) *((uint16_t *) &data[8]) * 1000LLU;
            reading->power_average     = (uint64_t) *((uint16_t *) &data[10]) * 1000LLU;
            reading->timestamp.tv_sec  = (uint64_t) *((uint32_t *) &data[12]);
            reading->timestamp.tv_nsec = 0;
            reading->timeframe         = (uint64_t) *((uint32_t *) &data[16]);
            break;
    }
}

static state_t energy_read(uint d, ipmi_cmd_t *pkg, consumption_t *rd)
{
    if (!send_oem_message(d, pkg)) {
        debug("ERROR while reading IPMI: %s", state_msg);
        return EAR_ERROR;
    }
    parse(opt[0], pkg->msg_recv_data, rd);
    debug("read->energy: %lu mJ", rd[d].energy);
    debug("read->power : %lu/%lu/%lu/%lu mW (cur/min/max/avg)", rd[d].power_current, rd[d].power_minimum,
          rd[d].power_maximum, rd[d].power_average);
    debug("read->tstamp: %ld s, %ld ns", rd[d].timestamp.tv_sec, rd[d].timestamp.tv_nsec);
    debug("read->tframe: %lu", rd[d].timeframe);
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
    state_t s        = EAR_SUCCESS;
    consumption_t rd = {0};
    ipmi_cmd_t pkg   = {0};
    double fpower;
    double ftime;
    int d;

    while (pthread_mutex_trylock(&lock))
        ;
    for (d = 0; d < devs_count; ++d) {
        // Cleaning and reading
        memset(&rd, 0, sizeof(consumption_t));
        if (state_fail(s = energy_read(d, &pkg, &rd))) {
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
        pool[d].energy        = (opt[1]) ? pool[d].energy + (uint64_t) fpower : rd.energy;
        pool[d].power_current = rd.power_current;
        pool[d].power_minimum = rd.power_minimum;
        pool[d].power_maximum = rd.power_maximum;
        pool[d].power_average = rd.power_average;
        pool[d].timeframe     = rd.timeframe;
        pool[d].timestamp     = rd.timestamp;
        pool[d].samples += 1;

        debug("pool->energy: %lu mJ", pool[d].energy);
        debug("pool->power : %lu/%lu/%lu/%lu mW (cur/min/max/avg)", pool[d].power_current, pool[d].power_minimum,
              pool[d].power_maximum, pool[d].power_average);
        debug("pool->tstamp: %ld s, %ld ns", pool[d].timestamp.tv_sec, pool[d].timestamp.tv_nsec);
        debug("pool->tframe: %lu", pool[d].timeframe);
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
