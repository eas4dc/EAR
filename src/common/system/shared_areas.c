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

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <common/config.h>
#include <common/states.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/system/shared_areas.h>



/** BASIC FUNCTIONS */

void *create_shared_area(char *path,char *data,int area_size,int *shared_fd,int must_clean)
{
	/* This function creates a shared memory region based on files and mmap */
	int ret;
	void * my_shared_region=NULL;		
	char buff[256];
	int fd;
	mode_t my_mask;
	if ((area_size==0) || (data==NULL) || (path==NULL)){
		//error("creatinf shared region, invalid arguments. Not created\n");
		return NULL;
	}
	my_mask=umask(0);
	strcpy(buff,path);
	verbose(VCONF+2,"creating file %s for shared memory\n",buff);
	fd=open(buff,O_CREAT|O_RDWR|O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd<0){
		//error("error creating sharing memory (%s)\n",strerror(errno));
		umask(my_mask);
		return NULL;
	}
	verbose(VCONF+2,"shared file for mmap created\n");
	umask(my_mask);
	// Default values
	if (must_clean) bzero(data,area_size);
	verbose(VCONF+2,"writting default values\n");
	ret=write(fd,data,area_size);
	if (ret<0){
		//error("error creating sharing memory (%s)\n",strerror(errno));
		close(fd);
		return NULL;
	}
	verbose(VCONF+2,"mapping shared memory\n");
	my_shared_region= mmap(NULL, area_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);                                     
	if ((my_shared_region == MAP_FAILED) || (my_shared_region == NULL)){
		error(" error creating sharing memory (%s)\n",strerror(errno));
		close(fd);
		return NULL;
	}
	*shared_fd=fd;
	return my_shared_region;
}

void * attach_shared_area(char *path,int area_size,uint perm,int *shared_fd,int *s)
{
    void * my_shared_region=NULL;
		char buff[256];
		int flags;
		int size;
		int fd;
		strcpy(buff,path);
    fd=open(buff,perm);
    if (fd<0){
        //error("error attaching to sharing memory (%s)\n",strerror(errno));
        return NULL;
    }
	if (area_size==0){
		size=lseek(fd,0,SEEK_END);
		if (size<0){
			//error("Error computing shared memory size (%s) ",strerror(errno));
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
        //error("error attaching to sharing memory (%s)\n",strerror(errno));
        close(fd);
        return NULL;
    }
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

