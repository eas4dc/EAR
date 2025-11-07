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

#include <common/output/debug.h>
#include <common/system/time.h>
#include <metrics/common/ipmi.h>

typedef struct power_data {
    uint64_t cur;
    uint64_t min;
    uint64_t max;
    uint64_t avg;
    uint64_t timeframe;
    timestamp_t timestamp; // We use our timestamp because DCMI fails
} power_data_t;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static suscription_t *sus;
static uint devs_count;
static uint opened;
static power_data_t power_last[IPMI_MAX_DEVS];
static power_data_t power_cur[IPMI_MAX_DEVS];
static double energy_acum; // mJ

static state_t power_reading(power_data_t *pr)
{
    ipmi_cmd_t pkg;
    state_t s;

    // Addresses and messages are shared among all devices
    ipmi_cmd_set_addr(&pkg, IPMI_SYSTEM_INTERFACE_ADDR_TYPE, IPMI_BMC_CHANNEL, 0x00, 0x00);
    ipmi_cmd_set_msg(&pkg, 0x2c, 0x02, 4);
    pkg->msg_send.data[0] = 0xdc;
    pkg->msg_send.data[1] = 0x01;
    pkg->msg_send.data[2] = 0x00;
    pkg->msg_send.data[3] = 0x00;

    for (d = 0; d < devs_count; ++d) {
        timestamp_get(&pr[d]->timestamp);
        if (state_fail(s = ipmi_cmd_send(d, &pkg))) {
            return s;
        }
        if (pkg.msg_recv_data[0] != 0) {
            return_msg(EAR_ERROR, "invalid completion code")
        }
        power_read[d]->cur = (uint64_t) *((uint16_t *) &rsp->data[1]);
        power_read[d]->min = (uint64_t) *((uint16_t *) &rsp->data[3]);
        power_read[d]->max = (uint64_t) *((uint16_t *) &rsp->data[5]);
        power_read[d]->avg = (uint64_t) *((uint16_t *) &rsp->data[7]);
        // power_read[d]->timestamp = (uint64_t) *((uint32_t *) &rsp->data[9]);
        power_read[d]->timeframe = (uint64_t) *((uint32_t *) &rsp->data[13]);
    }
}

static uint64_t energy_reading()
{
    uint64_t energy_acum = 0;
    double time, energy;
    int d;
    memcpy(power_last, power_cur, sizeof(power_data_t) * IPMI_MAX_DEVS) power_reading(power_cur);
    for (d = 0; d < devs_count; ++d) {
        time   = timestamp_fdiff(&power_cur[d].timestamp, &power_last[d].timestamp, TIME_SECS, TIME_MSECS);
        energy = ((double) power_cur.avg) * 1000.0 * time; // Energy is reported in mJ
        energy_acum += (uint64_t) energy;
    }
    return energy_acum;
}

static state_t thread_main(void *p)
{
    while (pthread_mutex_trylock(&lock))
        ;
    energy_acum += energy_reading;
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t energy_init(void **c)
{
    power_data_t pd[IPMI_MAX_DEVS];
    state_t s = EAR_SUCCESS;

    while (pthread_mutex_trylock(&lock))
        ;
    if (opened) {
        goto leave;
    }
    if (state_fail(s = ipmi_open())) {
        goto leave;
    }
    devs_count = ipmi_devs_count();
    //
    if (state_fail(s = power_reading(pd))) {
        goto leave;
    }
    sus             = suscription();
    sus->call_init  = NULL;
    sus->call_main  = thread_main;
    sus->time_relax = ear_max(pd[0].timeframe, 2000); // 2 seconds
    sus->time_burst = ear_max(pd[0].timeframe, 2000);
    sus->suscribe(sus);
    //
    opened = 1;
leave:
    pthread_mutex_unlock(&lock);
    return s;
}

state_t energy_dispose(void **c)
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (opened) {
        monitor_unregister(sus);
    }
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t energy_datasize(size_t *size)
{
}

state_t energy_frequency(ulong *freq_us)
{
}

state_t energy_to_str(char *str, void *energy_values)
{
}

state_t energy_units(uint *units)
{
    *units = 1000;
    return EAR_SUCCESS;
}

state_t energy_accumulated(uint64_t *energy, void *read1, void *read2)
{
    return EAR_SUCCESS;
}

state_t energy_dc_read(void *c, void *energy_mj)
{
    while (pthread_mutex_trylock(&lock))
        ;
    energy_acum += energy_reading;
    energy_mj = (void *) &energy_acum;
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t energy_dc_time_read(void *c, edata_t energy_mj, ulong *time_ms)
{
}

uint energy_data_is_null(edata_t e)
{
    ulong *pe = (ulong *) e;
    return (*pe == 0);
}