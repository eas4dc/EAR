/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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

