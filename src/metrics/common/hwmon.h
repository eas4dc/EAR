/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

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
 * Remember to close/free them. Unopened file descriptors value is '-1'. */
state_t hwmon_open_files(uint id, hwmon_t files, int **fds, uint *n);

/* */
state_t hwmon_close_files(int *fds, uint n);

state_t hwmon_read(int fd, char *buffer, size_t size);

#endif //METRICS_COMMON_HWMON
