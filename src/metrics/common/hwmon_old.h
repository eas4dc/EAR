/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_HWMON_OLD
#define METRICS_COMMON_HWMON_OLD

#include <common/states.h>
#include <common/types.h>

// This is a DEPCRECATED class and its use is not recommended.

struct Hwmon_s {
    char *temp_input;
    char *temp_max;
} Hwmon __attribute__((weak)) = {
    .temp_max   = "/sys/class/hwmon/hwmon%d/temp%d_max",
    .temp_input = "/sys/class/hwmon/hwmon%d/temp%d_input",
};

struct Hwmon_acpi_power {
    char *power_avg;
    char *power_avg_int;
} Hwmon_power_meter __attribute__((weak)) = {
    .power_avg     = "/sys/class/hwmon/hwmon%d/device/power%d_average",
    .power_avg_int = "/sys/class/hwmon/hwmon%d/device/power%d_average_interval",
};

struct Hwmon_pvc {
    char *energy_j;
} Hwmon_pvc_energy __attribute__((weak)) = {
    .energy_j = "/sys/class/hwmon/hwmon%d/energy%d_input",
};

/* Find a set of drivers by name, also allocates memory for its ids.
 * Remember to free them. */
state_t hwmon_find_drivers(const char *name, uint **ids, uint *n);

/* Finds a set of files by driver id, also allocates memory for its fds.
 * Remember to close/free them. Unopened file descriptors value is '-1'.
 * Files follow a pattern name_$, where $ is a number starting at i_start. */
state_t hwmon_open_files(uint id, char *files, int **fds, uint *n, int i_start);

/* */
state_t hwmon_close_files(int *fds, uint n);

state_t hwmon_read_files(int fd, char *buffer, size_t size);

#endif // METRICS_COMMON_HWMON