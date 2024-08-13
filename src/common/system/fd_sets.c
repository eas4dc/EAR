/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include <common/states.h>

void clean_fd_set(fd_set *set,int *total)
{
	FD_ZERO(set);
	*total = 0;
}
state_t new_fd(fd_set *set,int fd,int *max,int *total)
{
	if (*total >= FD_SETSIZE) return EAR_ERROR;
	FD_SET(fd,set);
	if (fd >= *max) *max = fd + 1;
	*total = *total +1;
	return EAR_SUCCESS;
}
state_t clean_fd(fd_set *set,int fd,int *max,int *total)
{
	int i,new_max;
	FD_CLR(fd,set);
	*total = *total - 1;
	if (fd == (*max -1)){
		new_max=-1;
		for (i=0;i<FD_SETSIZE;i++){
			if (FD_ISSET(i,set)){
				if (i>new_max) new_max = i;
			}
		}
		*max = new_max +1;
	}
	return EAR_SUCCESS;
}

state_t check_fd(fd_set *set,int fd)
{
	struct stat my_stat;
	if (FD_ISSET(fd,set)){
		if (fstat(fd,&my_stat) < 0) {
			return EAR_ERROR;
		}
	}
	return EAR_SUCCESS;
}
