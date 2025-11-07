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
#include <metrics/common/hwmon.h>
#include <metrics/temperature/archs/hwmon.h>

static hwmon_t *chips;
static uint chips_count;
static uint sockets_count;
static uint items_count;
static char **labels_test;
static char *labels_amd17[]   = {"Tccd", "Tctl", NULL}; // Tctl is the real temperature plus an offset
static char *labels_intel63[] = {"Package", NULL};
static char *label;
static char *driver_name;

TEMP_F_LOAD(hwmon)
{
    // Another API is opened and this API is not mixtable
    if (ops->read != NULL) {
        return;
    }
    if (tp->vendor == VENDOR_AMD && tp->family >= FAMILY_ZEN) {
        driver_name = "k10temp";
        labels_test = labels_amd17;
    } else if (tp->vendor == VENDOR_INTEL && tp->model >= MODEL_ICELAKE_X) {
        driver_name = "coretemp";
        labels_test = labels_intel63;
    } else {
        return;
    }
    if (state_fail(hwmon_open(driver_name, "temp", NULL, &chips, &chips_count))) {
        debug("Failed: %s", state_msg);
        return;
    }
    debug("HWMON opened");
    while (labels_test != NULL && *labels_test != NULL) {
        debug("Counting label '%s'", *labels_test);
        if ((items_count = hwmon_count_items(chips, "input", *labels_test)) > 0) {
            label = *labels_test;
            break;
        }
        labels_test++;
    }
    if (label == NULL) {
        hwmon_close(&chips);
        return;
    }
    sockets_count = tp->socket_count;
    debug("Detected %u HWMON '%s' chips with %u items in a system with %d sockets",
          chips_count, label, items_count, sockets_count);
    apis_put(ops->get_info, temp_hwmon_get_info);
    apis_put(ops->init    , temp_hwmon_init);
    apis_put(ops->dispose , temp_hwmon_dispose);
    apis_put(ops->read    , temp_hwmon_read);
}

TEMP_F_GET_INFO(hwmon)
{
    info->api         = API_HWMON;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_SOCKET; // We use average per chip, and a chip is per socket
    info->devs_count  = sockets_count;
}

TEMP_F_INIT(hwmon)
{
    // Nothing
    return EAR_SUCCESS;
}

TEMP_F_DISPOSE(hwmon)
{
    if (chips != NULL) {
        hwmon_close(&chips);
        chips = NULL;
    }
}

TEMP_F_READ(hwmon)
{
    llong temp_in;
    llong temp_av;
    hwmon_t *chip;
    int c = 0;

    if (average != NULL) {
        *average = 0LL;
    }
    // Reading and calculating the average per chip
    hwmon_read(chips);
    hwmon_calc_average(chips, label);
    while ((chip = hwmon_iter_chips(chips)) != NULL) {
        temp_in = ((llong) chip->devs_avg.input_toint) / 1000LL;
        temp_av = ((llong) chip->devs_avg.input_toint) / 1000LL;
        if (temp != NULL) temp[c] = temp_in;
        if (average != NULL) *average += temp_av;
        debug("Read temp %lld", temp_in);
        ++c;
    }
    debug("----");
    if (average != NULL) {
        *average /= (llong) c;
    }
    return EAR_SUCCESS;
}