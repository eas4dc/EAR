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

#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/monitor.h>
#include <metrics/common/hwmon.h>
#include <metrics/energy_cpu/archs/hwmon.h>

#define MONITORING_TIME 2

typedef struct chips_s {
    hwmon_t *chips;
    uint     chips_count;
    ullong  *energy_uj; // Accumulated value, per chip
    double  *power_uw_last; // Last valid value, per chip
} chips_t;

static chips_t         chips_socket;
static chips_t         chips_pkg;
static chips_t         chips_sysio;
static suscription_t  *sus;
static timestamp_t     ts_last;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void chips_open(chips_t *chips, char *label)
{
    hwmon_open("power_meter", "power", label, &chips->chips, &chips->chips_count);
    chips->energy_uj     = calloc(chips_pkg.chips_count, sizeof(ullong));
    chips->power_uw_last = calloc(chips_pkg.chips_count, sizeof(double));
    debug("%s #chips: %u", label, chips->chips_count);
}

static void chips_close(chips_t *chips)
{
    if (chips != NULL) {
        hwmon_close(&chips->chips);
        free(chips->power_uw_last);
        free(chips->energy_uj);
        chips->chips_count = 0;
        chips->chips = NULL;
    }
}

static state_t chips_close_all(state_t r)
{
    chips_close(&chips_socket);
    chips_close(&chips_pkg);
    chips_close(&chips_sysio);
    return r;
}

state_t energy_cpu_hwmon_load(topology_t *tp_in)
{
    uint items_pkg_count;

    if (tp_in->vendor == VENDOR_ARM && tp_in->model == MODEL_NEOVERSE_V2) {
        chips_open(&chips_socket, "Grace Power Socket");
        chips_open(&chips_pkg   , "CPU Power Socket"  ); // Includes caches, so it is package
        chips_open(&chips_sysio , "SysIO Power Socket");
    } else {
        chips_open(&chips_pkg, NULL); // Is like acpi_power.c
    }
    if (!chips_pkg.chips_count) {
        return chips_close_all(EAR_ERROR);
    }
    if ((items_pkg_count = hwmon_count_items(chips_pkg.chips, "average", NULL)) == 0) {
        return chips_close_all(EAR_ERROR);
    }
    timestamp_get(&ts_last);
    return EAR_SUCCESS;
}

static void static_read_single(chips_t *chips, double time_s)
{
    double power_uw = 0.0;
    hwmon_t *chip = NULL;
    ullong aux_uw = 0.0;
    int i = 0;

    if (chips == NULL) {
        return;
    }
    hwmon_read(chips->chips);
    hwmon_calc_average(chips->chips, NULL);
    while ((chip = hwmon_iter_chips(chips->chips)) != NULL) {
        // If returned 0
        if ((power_uw = (double) chip->devs_avg.average_toint) < 0.001) {
            power_uw = chips->power_uw_last[i];
        } else {
            chips->power_uw_last[i] = power_uw;
        }
        // grace_cpu_power.c class is calculating time in mS, so we add another 1000.0 to copy its behaviour
        aux_uw = (ullong) (power_uw * time_s * 1000.0);
        chips->energy_uj[i] += aux_uw;
        debug("Process %d: CHIP[%d]: %llu += %llu = %0.lf * (%0.3lf us) (%s)",
            getpid(), i, chips->energy_uj[i], aux_uw, power_uw, time_s, chips->chips[0].devs[0].label);
        ++i;
    }
}

static state_t static_read()
{
    timestamp_t ts;
    double time_s = 0.0;

    timestamp_get(&ts);
    if ((time_s = timestamp_fdiff(&ts, &ts_last, TIME_SECS, TIME_MSECS)) < 0.1) {
        return EAR_SUCCESS;
    }
    static_read_single(&chips_socket, time_s);
    static_read_single(&chips_pkg   , time_s);
    static_read_single(&chips_sysio , time_s);
    ts_last = ts;
    return EAR_SUCCESS;
}

static state_t static_pool(void *data)
{
    ullong *values = (ullong *) data;
    int j;

    while (pthread_mutex_trylock(&lock));
    static_read(); // converted to micro Watts
    // It is 2 because this apis is DRAM[0]+PKG[1]
    for (j = 0; data != NULL && j < chips_pkg.chips_count; j += 1) {
        values[j+0] = 0LLU;
        if (chips_socket.energy_uj[j] != 0 && chips_pkg.energy_uj[j] != 0 && chips_sysio.energy_uj[j] != 0) {
            debug("vector [%d] info [%d]", j, j)
            values[j+0] = chips_socket.energy_uj[j] - (chips_pkg.energy_uj[j] + chips_sysio.energy_uj[j]);
            if (values[j+0] >= chips_socket.energy_uj[j]) {
                values[j+0] = 0LLU;
            }
        }
        debug("vector [%d] info [%d]", j+chips_pkg.chips_count, j)
        values[j + chips_pkg.chips_count] = chips_pkg.energy_uj[j];
    }
    pthread_mutex_unlock(&lock);
    return EAR_SUCCESS;
}

state_t energy_cpu_hwmon_init(ctx_t *c)
{
    if (sus == NULL) {
        sus             = suscription();
        sus->call_main  = static_pool;
        sus->call_init  = NULL;
        sus->time_relax = MONITORING_TIME * 1000;
        sus->time_burst = MONITORING_TIME * 1000;
        sus->suscribe(sus);
    }
    return EAR_SUCCESS;
}

state_t energy_cpu_hwmon_dispose(ctx_t *c)
{
    if (sus != NULL) {
        monitor_unregister(sus);
        sus = NULL;
    }
    return chips_close_all(EAR_SUCCESS);
}

state_t energy_cpu_hwmon_count_devices(ctx_t *c, uint *devs_count_in)
{
    *devs_count_in = chips_pkg.chips_count;
    return EAR_SUCCESS;
}

state_t energy_cpu_hwmon_read(ctx_t *c, ullong *values)
{
    static_pool(values);
    return EAR_SUCCESS;
}
