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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <common/output/verbose.h>

int new_service(char *service)
{
	char service_name[128];
	int must_recover=0;
	int fd,pid;
	
	sprintf(service_name,"/var/run/%s.pid",service);
	verbose(0,"Checking %s file",service_name);
    	pid=getpid();

	if ((fd=open(service_name,O_RDWR))>=0){
    	write(fd,&pid,sizeof(int));
		must_recover=1;
		close(fd);
    }else{
        verbose(1,"%s file doesn't exist,creating it",service_name);
        fd=open(service_name,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
        if (fd<0){
            verbose(0,"Error, %s file cannot be created (%s)",service_name,strerror(errno));
        }else{
            write(fd,&pid,sizeof(int));
            close(fd);
        }
    }
    return must_recover;
}

void end_service(char *name)
{
	char service_name[128];
	sprintf(service_name,"/var/run/%s.pid",name);
    unlink(service_name);
}

