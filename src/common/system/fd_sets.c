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
