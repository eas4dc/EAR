/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
