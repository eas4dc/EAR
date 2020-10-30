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
