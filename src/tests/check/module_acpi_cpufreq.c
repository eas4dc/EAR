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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <linux/limits.h>
#include <common/string_enhanced.h>
#include <tests/check/check.h>

int main ()
{
	char path[PATH_MAX];
	char line[16];
	FILE *fd;

	// General test
	assert(find_module("acpi_cpufreq"));

	// Test if scaling is supported
	sprintf(path, "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed");
    fd = fopen(path, "r");
	
	assert(fd != NULL);
	assert(fgets (line, 16, fd) != NULL);
	
	strclean(line, '\n');
	assert(strcmp("<unsupported>", line) != 0);

	fclose(fd);

	// Test if the scaling driver is effectively acpi-cpufreq
	sprintf(path, "/sys/devices/system/cpu/cpu0/cpufreq/scaling_driver");
    fd = fopen(path, "r");
    
	assert(fd != NULL);
    assert(fgets (line, 16, fd) != NULL);

	strclean(line, '\n');
    assert(strcmp("acpi-cpufreq", line) == 0);
 
	fclose(fd);
 
    return 0;
}
