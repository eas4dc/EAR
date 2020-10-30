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

#ifndef EAR_PROCESS_H
#define EAR_PROCESS_H

#include <common/sizes.h>
#include <common/states.h>

typedef struct process_data
{
	char path_pid[SZ_PATH];
	char name[SZ_NAME_SHORT];
	pid_t pid;
} process_data_t;

/* */
void process_data_initialize(process_data_t *prodata, char *name, char *path_pid);

/* */
void process_update_pid(process_data_t *prodata);

/* */
int process_exists(const process_data_t *prodata, char *bin_name, pid_t *pid);

/* */
state_t process_pid_file_save(const process_data_t *prodata);

/* */
state_t process_pid_file_load(const process_data_t *prodata, pid_t *pid);

/* */
state_t process_pid_file_clean(process_data_t *prodata);

#endif //EAR_PROCESS_H
