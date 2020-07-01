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


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <common/config.h>


int create_log(char *path,char *service_name)
{
        int fd;
        char logfile[256];
        mode_t my_mask;
        my_mask=umask(0);

        sprintf(logfile,"%s/%s.log",path,service_name);
        unlink(logfile);
        //fd=open(logfile,O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        fd=open(logfile,O_CREAT|O_WRONLY|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        chmod(logfile,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        
        umask(my_mask);

        return fd;
}

