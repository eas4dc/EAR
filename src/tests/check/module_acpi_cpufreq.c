/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017  
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*	
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*	
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING	
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
