/**************************************************************
*   Energy Aware Runtime (EAR)
*   This program is part of the Energy Aware Runtime (EAR).
*
*   EAR provides a dynamic, transparent and ligth-weigth solution for
*   Energy management.
*
*       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*   BSC Contact     mailto:ear-support@bsc.es
*   Lenovo contact  mailto:hpchelp@lenovo.com
*
*   EAR is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Lesser General Public
*   License as published by the Free Software Foundation; either
*   version 2.1 of the License, or (at your option) any later version.
*
*   EAR is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public
*   License along with EAR; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*   The GNU LEsser General Public License is contained in the file COPYING
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

