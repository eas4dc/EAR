/*********************************************************************
 * Copyright (c) 2024 Energy Aware Solutions, S.L
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **********************************************************************/

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <common/math_operations.h>
#include <common/hardware/topology.h>
#include <metrics/common/hwmon.h>

#define MONITORING_TIME 2

static hwmon_t       *chips;
static uint           chips_count;
static uint           items_count;
static suscription_t *sus;
static ullong        *energy_uj;
static double        *power_uw_last; // Last valid value
static char          *label;
static timestamp_t ts_last;
static topology_t     tp;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static state_t static_pool(void *data);

static state_t static_monitor_init()
{
    if (sus == NULL) {
        debug("Subscription done");
        sus             = suscription();
        sus->call_main  = static_pool;
        sus->call_init  = NULL;
        sus->time_relax = MONITORING_TIME * 1000;
        sus->time_burst = MONITORING_TIME * 1000;
        sus->suscribe(sus);
    }
    return EAR_SUCCESS;
}

state_t energy_init(void **c)
{
    debug("energy_init");
    topology_init(&tp);
    if (tp.vendor == VENDOR_ARM && tp.model == MODEL_NEOVERSE_V2) {
	debug("VENDOR_ARM && MODEL_NEOVERSE_V2");
        if (tp.socket_count > 1) {
            label = "Grace Power Socket"; // Energy is in micro Watts
        } else {
            label = "Module Power Socket"; // Energy is in micro Watts
        }
    } else {
        // By now, when "power_meter" is found with no label, it only has one
        // chip and one device (node manager), and returns the node consumption.
    }
    debug("Using label:%s", label);
    if (state_fail(hwmon_open("power_meter", "power", label, &chips, &chips_count))) {
	debug("hwmon_open for power_meter failed");
        return EAR_ERROR;
    }
    if ((items_count = hwmon_count_items(chips, "average", label)) == 0) {
	debug("No items found");
        hwmon_close(&chips);
        return EAR_ERROR;
    }
    debug("chips_count  : %d", chips_count);
    debug("items_count  : %d", items_count);
    debug("sockets_count: %d", tp.socket_count);
    energy_uj = calloc(1, sizeof(ullong)); // Single value because is the whole node
    power_uw_last = calloc(chips_count, sizeof(double)); // Per chip
    timestamp_get(&ts_last);
    return static_monitor_init();
}

static state_t static_read(ullong *value_uj)
{
    double power_uw;
    double time_s;
    timestamp_t ts;
    hwmon_t *chip;
    ullong aux;
    int i = 0;

    debug("static_read");
    hwmon_read(chips);
    hwmon_calc_average(chips, NULL);
    timestamp_get(&ts);
    if ((time_s = timestamp_fdiff(&ts, &ts_last, TIME_SECS, TIME_MSECS)) < 0.1) {
        return EAR_SUCCESS;
    }
    while ((chip = hwmon_iter_chips(chips)) != NULL) {
        // If returned 0
        if ((power_uw = (double) chip->devs_avg.average_toint) < 0.001) {
            power_uw = power_uw_last[i];
        } else {
            power_uw_last[i] = power_uw;
        }
        // Converted to micro Joules
        aux = (ullong) (power_uw * time_s);
        *value_uj += aux;
        //*value_uj += ((ullong) (power_uw)) * MONITORING_TIME;
        debug("CHIP[%d]: %llu += %llu = %0.lf * (%0.3lf us)", i, *value_uj, aux, power_uw, time_s);
        i++;
    }
    ts_last = ts;
    return EAR_SUCCESS;
}

static state_t static_pool(void *data)
{
    debug("static_pool");
    while (pthread_mutex_trylock(&lock));
    static_read(energy_uj);
    if (data != NULL) {
        memcpy(data, energy_uj, sizeof(ullong)*1);
    }
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t energy_dispose(void **c)
{
    if (sus != NULL) {
        monitor_unregister(sus);
        sus = NULL;
    }
    if (chips != NULL) {
        hwmon_close(&chips);
        chips = NULL;
    }
    return EAR_SUCCESS;
}

state_t energy_dc_read(void *c, void *data)
{
    static_pool(data);
    return EAR_SUCCESS;
}

state_t energy_datasize(size_t *size)
{
    *size = sizeof(ullong) * chips_count;
    return EAR_SUCCESS;
}

state_t energy_units(uint *units)
{
    *units = 1000; // mJ
    return EAR_SUCCESS;
}

state_t energy_accumulated(ulong *energy_mj, void *data1, void *data2)
{
    ullong *readings2 = (ullong *) data2;
    ullong *readings1 = (ullong *) data1;

    *energy_mj = 0LU;
    if (readings2 != 0LLU && readings1 != 0LLU) {
        *energy_mj += (ulong) (overflow_zeros_u64(readings2[0], readings1[0]) / 1000LLU); // Converting to mili Joules
        debug("energy: %lu mJ (%llu uJ - %llu uJ)", *energy_mj, readings2[0], readings1[0]);
    }
    return EAR_SUCCESS;
}

state_t energy_to_str(char *buffer, void *data)
{
    sprintf(buffer, "%llu", *((ullong *) data));
    return EAR_SUCCESS;
}

uint energy_data_is_null(void *data)
{
    return ((ullong *) data) == 0LLU;
}

state_t energy_not_privileged_init()
{
    return energy_init(NULL);
}

uint energy_is_privileged()
{
    // This function needs more robustness.
    // We should call the static read and check state
    return 0;
}
