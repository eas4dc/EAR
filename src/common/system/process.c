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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <common/system/file.h>
#include <common/sizes.h>
#include <common/system/process.h>

void process_data_initialize(process_data_t *prodata, char *name, char *path_pid)
{
	sprintf(prodata->path_pid, "%s/%s.pid", path_pid, name);
	strcpy(prodata->name, name);
	process_update_pid(prodata);
}

void process_update_pid(process_data_t *prodata)
{
	prodata->pid = getpid();
}

int process_exists(const process_data_t *prodata, char *bin_name, pid_t *pid)
{
	int value = 0;
	state_t state;

	//
	state = process_pid_file_load(prodata, pid);

	if (state_fail(state)) {
		return 0;
	}

	//
	value = !((kill(*pid, 0) < 0) && (errno == ESRCH));

	if (value)
	{
		char *buffer1 = malloc(SZ_PATH);
		char *buffer2 = malloc(SZ_PATH);
		char *p;

		sprintf(buffer1, "/proc/%d/cmdline", *pid);
		file_read(buffer1, buffer2, SZ_PATH);
		p = strstr(buffer2, bin_name);

		free(buffer1);
		free(buffer2);

		return (p != NULL);
	}

	return 0;
}

state_t process_pid_file_save(const process_data_t *prodata)
{
	char buffer[SZ_NAME_SHORT];
	state_t state;

	sprintf(buffer, "%d\n", prodata->pid);
	state = file_write(prodata->path_pid, buffer, strlen(buffer));

	if (state_fail(state)) {
		return state;
	}

	state_return(EAR_SUCCESS);
}

state_t process_pid_file_load(const process_data_t *prodata, pid_t *pid)
{
	char buffer[SZ_NAME_SHORT];
	state_t state;

	state = file_read(prodata->path_pid, buffer, SZ_NAME_SHORT);

	if (state_fail(state)) {
		state_return(state);
	}

	*pid = (pid_t) atoi(buffer);
	state_return(EAR_SUCCESS);
}

state_t process_pid_file_clean(process_data_t *prodata)
{
	file_clean(prodata->path_pid);
	state_return(EAR_SUCCESS);
}
