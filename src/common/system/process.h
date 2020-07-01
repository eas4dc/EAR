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
