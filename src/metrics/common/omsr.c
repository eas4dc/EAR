/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <common/sizes.h>
#include <metrics/common/omsr.h>
#include <common/hardware/hardware_info.h>
//#define SHOW_DEBUGS 0
#include <common/output/verbose.h>

/* */
static int total_packages=0;
static int *package_map;
static int msr_initialised = 0;
static int fd_map[MAX_PACKAGES];

int is_msr_initialized()
{
	return msr_initialised;
}

int get_msr_ids(int *dest_fd_map)
{
	/* If msr was not initialized, we initialize and made a local copy of fds */
	if (!msr_initialised){
		init_msr(dest_fd_map);
		memcpy(fd_map,dest_fd_map,sizeof(int)*get_total_packages());
	}else{	
		memcpy(dest_fd_map,fd_map,sizeof(int)*get_total_packages());
	}
	return EAR_SUCCESS;
}

int get_total_packages()
{
	return total_packages;
}

/* It is supposed to be checked it is not already initialized before calling it */
int init_msr(int *dest_fd_map)
{
	total_packages = detect_packages(&package_map);

	if (total_packages==0)
	{
		debug("Totall packages detected in init_msr is zero");	
		return EAR_ERROR;
	}

	int j;
	
	for(j=0;j<total_packages;j++) 
	{
        	int ret;
        	fd_map[j] = -1;
		if ((ret = omsr_open(package_map[j], &fd_map[j])) != EAR_SUCCESS) {   
  	    		return EAR_ERROR;
		}
	}

	memcpy(dest_fd_map,fd_map,sizeof(int)*total_packages);
	msr_initialised = 1;

	return EAR_SUCCESS;
}

state_t omsr_open(uint cpu, int *fd)
{
	char msr_file_name[SZ_PATH_KERNEL];

	if (*fd >= 0) {
		debug("omsr_open fd already in use:EAR_BUSY");
		return EAR_BUSY;
	}

	sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
	*fd = open(msr_file_name, O_RDWR);
	
	if (*fd < 0)
	{
		char msr_safe_file_name[SZ_PATH_KERNEL];
		sprintf(msr_safe_file_name, "/dev/cpu/%d/msr_safe", cpu);
		*fd = open(msr_safe_file_name, O_RDWR);
		debug("Error when opening %s/%: %s",msr_file_name,msr_safe_file_name, strerror(errno));
		return EAR_OPEN_ERROR;
	}
	msr_initialised++;
	return EAR_SUCCESS;
}

/* */
state_t omsr_close(int *fd)
{
	if (*fd < 0) {
		return EAR_ERROR;
	}
	if (msr_initialised>0) msr_initialised--;
	close(*fd);
	*fd = -1;

	return EAR_SUCCESS;
}

/* */
state_t omsr_read(int *fd, void *buffer, size_t size, off_t offset)
{
	int ret;
	char *b=(char *)buffer;
	size_t pending=size;
	off_t total=0;
	if (*fd < 0) {
		return EAR_NOT_INITIALIZED;
	}
	do{
		ret=pread(*fd, (void*)&b[total], pending, offset+total);
		if (ret>=0) pending-=ret;
		total+=ret;	
	}while((ret>=0) && (pending>0));
	if (ret<0){
		debug("omsr_read returns error %s",strerror(errno));
		return EAR_READ_ERROR;
	}
	if (pending==0) return EAR_SUCCESS;
	return EAR_READ_ERROR;
}

/* */
state_t omsr_write(int *fd, const void *buffer, size_t size, off_t offset)
{
	int ret;
	char *b=(char *)buffer;
  size_t pending=size;
  off_t total=0;
	if (*fd < 0) {
		return EAR_NOT_INITIALIZED;
	}
	do{
		ret=pwrite(*fd, (void*)&b[total], pending, offset+total);
		if (ret>=0) pending-=ret;
		total+=ret;	
	}while((ret>=0) && (pending>0));
	if (ret<0){
		debug("omsr_write returns error %s",strerror(errno));
		return EAR_ERROR;
	}
	if (pending==0) return EAR_SUCCESS;
	return EAR_ERROR;
}
