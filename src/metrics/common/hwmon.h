/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_HWMON
#define METRICS_COMMON_HWMON

#include <metrics/common/hwmon_old.h>

/*  About HWMON, the HWMON manual says:
    There is only one value per file, unlike the older /proc specification.
    The common scheme for files naming is: <type><number>_<item>. Usual
    types for sensor chips are "in" (voltage), "temp" (temperature) and
    "fan" (fan). Usual items are "input" (measured value), "max" (high
    threshold, "min" (low threshold). Numbering usually starts from 1,
    except for voltages which start from 0 (because most data sheets use
    this). A number is always used for elements that can be present more
    than once, even if there is a single element of the given type on the
    specific chip. Other files do not refer to a specific element, so
    they have a simple name, and no number.
 */

#define HWMON_DEV_TYPE_TEMP   "temp"
#define HWMON_DEV_TYPE_POWER  "power"
#define HWMON_DEV_TYPE_ENERGY "energy"

#define hwmon_add(var)                                                                                                 \
    int var##_fd;                                                                                                      \
    char var[32];                                                                                                      \
    int var##_toint;

typedef struct hwmon_dev_s {
    hwmon_add(input);
    hwmon_add(label);
    hwmon_add(max);
    hwmon_add(min);
    hwmon_add(average);
    hwmon_add(average_interval);
    char is_visited;
    char is_null;
    char number;
} hwmon_dev_t;

typedef struct hwmon_s {
    hwmon_dev_t devs_avg;
    hwmon_dev_t *devs;
    uint devs_count;
    char is_visited;
    char is_null;
} hwmon_t;

// It returns an array of hwmon_t structs based on chip name like "coretemp" or
// "k10temp" and a device type such as "temp", "power" or "energy". In this
// array there is a position for each chip found: one per hwmon folder whose
// name is equal the name you passed. In each array position you can find also a
// list of devices based on the device type you passed (i.e. "temp"), whose
// variables are the items of the devices (i.e. input, max, label, etc). If
// label is not NULL, it will open only devices that contains the label string.
state_t hwmon_open(char *chip_name, char *dev_type, char *label, hwmon_t *chips[], uint *chips_count);

void hwmon_close(hwmon_t *chips[]);

void hwmon_read(hwmon_t chips[]);

// This function counts the number of items found (i.e. "input"). Also you can
// pass a label to count items per label. It accepts NULL labels. If you pass
// "input" for an opened "temp" device type, it will count all the devices that
// have tempXX_input items.
int hwmon_count_items(hwmon_t chips[], char *item_name, char *label);

// The iter functions iterate over all chips and devices until returns NULL.
// When called again they start from the beginning.
hwmon_t *hwmon_iter_chips(hwmon_t chips[]);

hwmon_dev_t *hwmon_iter_devs(hwmon_t *chip, char *label);

// It calculates the average of the all the devices per chip, based in a label
// (i.e. "Core"). If label is NULL, all devices are used to compute the chip
// average. The values are saved in the devs_avg device. The returned device
// is the average of all devices of all chips.
void hwmon_calc_average(hwmon_t chips[], char *label);

// Closes all file descriptors of files that are related to this label (or NOT in case you start the label with symbol '!')
void hwmon_close_labels(hwmon_t chips[], char *label);

#endif // METRICS_COMMON_HWMON
