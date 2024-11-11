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

#include <common/types.h>
#include <common/states.h>

typedef const char *hwmon_t;

struct Hwmon_s
{
	hwmon_t temp_input;
    hwmon_t temp_max;
}
Hwmon __attribute__((weak)) =
{
    .temp_max   = "/sys/class/hwmon/hwmon%d/temp%d_max",
    .temp_input = "/sys/class/hwmon/hwmon%d/temp%d_input",
};

/* Find a set of drivers by name, also allocates memory for its ids.
 * Remember to free them. */
state_t hwmon_find_drivers(const char *name, uint **ids, uint *n);

/* Finds a set of files by driver id, also allocates memory for its fds.
 * Remember to close/free them. Unopened file descriptors value is '-1'.
 * Files follow a pattern name_$, where $ is a number starting at i_start. */
state_t hwmon_open_files(uint id, hwmon_t files, int **fds, uint *n, int i_start);

/* */
state_t hwmon_close_files(int *fds, uint n);

state_t hwmon_read(int fd, char *buffer, size_t size);

#endif //METRICS_COMMON_HWMON
