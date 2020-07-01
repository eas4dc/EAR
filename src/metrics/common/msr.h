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

#ifndef EAR_MSR_H
#define EAR_MSR_H
#include <unistd.h>
#include <common/states.h>
#include <common/types/generic.h>

/* */

#define MAX_PACKAGES 16
#define NUM_SOCKETS 2

state_t msr_open(uint cpu, int *fd);

/* */
state_t msr_close(int *fd);

/* */
state_t msr_read(int *fd, void *buffer, size_t count, off_t offset);

/* */
state_t msr_write(int *fd, const void *buffer, size_t count, off_t offset);


int get_msr_ids(int *dest_fd_map);
int get_total_packages();
int is_msr_initialized();
int init_msr(int *dest_fd_map);

#endif //EAR_MSR_H
