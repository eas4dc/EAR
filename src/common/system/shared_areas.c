/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/config.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <common/system/shared_areas.h>



/** BASIC FUNCTIONS */


void *create_shared_area(char *path, mode_t file_perms_bits, char *data, int area_size, int *shared_fd, int must_clean)
{
	if (!area_size || !data || !path)
	{
		debug("creating shared region, invalid arguments. Not created\n");
		return NULL;
	}

	int ret;
	void *my_shared_region = NULL;		
	char buff[MAX_PATH_SIZE];
	int fd;

	mode_t my_mask;
	my_mask = umask(0);

	strncpy(buff, path, sizeof(buff) - 1);
	debug("creating file (%s) for shared memory", buff);

	fd = open(buff, O_CREAT | O_RDWR | O_TRUNC, file_perms_bits);
	if (fd < 0)
	{
		debug("error creating sharing memory (%s)\n", strerror(errno));
		umask(my_mask);
		return NULL;
	}

	chmod(buff, file_perms_bits);
	debug("shared file for mmap created with fd=%d", fd);

	umask(my_mask);


  // Default values
  if (must_clean) bzero(data, area_size);

  debug("writting default values");
  ret = write(fd, data, area_size);

  if (ret<0)
  {
    debug("error creating sharing memory (%s)\n", strerror(errno));
    close(fd);
    return NULL;
  }

	debug("mapping shared memory");
	my_shared_region= mmap(NULL, area_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);                                     
	if (my_shared_region == MAP_FAILED || my_shared_region == NULL)
	{
		debug(" error creating sharing memory (%s)\n", strerror(errno));
		close(fd);
		return NULL;
	}

	*shared_fd = fd;
	return my_shared_region;
}


void *attach_shared_area(char *path, int area_size, uint perm, int *shared_fd, int *s)
{
    void *my_shared_region = NULL;
		char buff[MAX_PATH_SIZE];
		int flags;
		int size;
		int fd;

		strncpy(buff, path, sizeof(buff) - 1);
    fd = open(buff, perm);
    *shared_fd = fd;

    if (fd < 0)
		{
#if SHOW_DEBUGS
			error("error attaching to shared memory at (%s): %s", buff, strerror(errno));
  		char test[2048];
  		sprintf(test,"ls -la %s", buff);
  		system(test);

#endif
				return NULL;
    }
		fsync(fd);
	debug("File for shared region %s opened", path);
	if (area_size==0){
		size=lseek(fd,0,SEEK_END);
		if (size<0){
			debug("Error computing shared memory size (%s) ",strerror(errno));
		}else area_size=size;
		if (s!=NULL) *s=size;
	}
	if (perm==O_RDWR){
		flags=PROT_READ|PROT_WRITE;
	}else{
		flags=PROT_READ;
	}
    my_shared_region= mmap(NULL, area_size,flags, MAP_SHARED, fd, 0);
    if ((my_shared_region == MAP_FAILED) || (my_shared_region == NULL)){
        debug("error attaching to sharing memory (%s)\n",strerror(errno));
        close(fd);
        return NULL;
    }
	debug("File for shared region %s mapped at %p", path, my_shared_region);
	*shared_fd=fd;
    return my_shared_region;
}

void dettach_shared_area(int fd)
{
	close(fd);
}
void dispose_shared_area(char *path,int fd)
{
	close(fd);
	unlink(path);
}

