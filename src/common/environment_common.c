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

#define DEFAULT_VERBOSE                 0
#define DEFAULT_DB_PATHNAME             ".ear_system_db"

char *conf_ear_tmp=NULL;
#if 0
char *conf_ear_db_pathname=NULL;
#endif
int conf_ear_verbose=DEFAULT_VERBOSE;

char * getenv_ear_tmp()
{
	char *my_ear_tmp;
	my_ear_tmp=getenv("EAR_TMP");
	if (my_ear_tmp==NULL){
		my_ear_tmp=getenv("TMP");
		//??
		if (my_ear_tmp==NULL) my_ear_tmp=getenv("HOME");
	}
	conf_ear_tmp=malloc(strlen(my_ear_tmp)+1);
	strcpy(conf_ear_tmp,my_ear_tmp);
	return conf_ear_tmp;	
}
#if 0
char *getenv_ear_db_pathname()
{
	char *my_ear_db_pathname = getenv("EAR_DB_PATHNAME");

	if (my_ear_db_pathname != NULL && strcmp(my_ear_db_pathname,"") != 0)
	{
		conf_ear_db_pathname = malloc(strlen(my_ear_db_pathname)+1);
		strcpy(conf_ear_db_pathname,my_ear_db_pathname);
	}

	return conf_ear_db_pathname;
}
#endif

int getenv_ear_verbose()
{
	char *my_verbose;
	my_verbose=getenv("EAR_VERBOSE");
	if (my_verbose!=NULL){
		conf_ear_verbose=atoi(my_verbose);
		if ((conf_ear_verbose<0) || (conf_ear_verbose>4)) conf_ear_verbose=DEFAULT_VERBOSE;
	}	
	return conf_ear_verbose;
}

char *get_ear_install_path()
{
	return getenv("EAR_INSTALL_PATH");
}

// get_ functions must be used after getenv_
char * get_ear_tmp()
{
	return conf_ear_tmp;
}
void set_ear_tmp(char *new_tmp)
{
	if (conf_ear_tmp!=NULL) free(conf_ear_tmp);
	conf_ear_tmp=malloc(strlen(new_tmp)+1);
	strcpy(conf_ear_tmp,new_tmp);
}
#if 0
char *get_ear_db_pathname()
{
	return conf_ear_db_pathname;
}
#endif

int get_ear_verbose()
{
	return conf_ear_verbose;
}

void set_ear_verbose(int verb)
{
	conf_ear_verbose=verb;
}

void ear_daemon_environment()
{
    getenv_ear_verbose();
    getenv_ear_tmp();
    // getenv_ear_db_pathname();
}
void ear_print_daemon_environment()
{
#if DEBUG
    char *tmp;
    char environ[256];
    char var[256];
    int fd;
    tmp=get_ear_tmp();
    sprintf(environ,"%s/ear_daemon_environment.txt",tmp);
    fd=open(environ,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd<0){
        verbose(0, "EAR error reporting environment variables %s", strerror(errno)); //error
        return;
    }
    sprintf(var,"EAR_TMP=%s\n",get_ear_tmp());
    write(fd,var,strlen(var));
    sprintf(var,"EAR_VERBOSE=%d\n",get_ear_verbose());
    write(fd,var,strlen(var));
	#if 0
    sprintf(var,"EAR_DB_PATHNAME=%s\n",get_ear_db_pathname());
    write(fd,var,strlen(var));
	#endif
    close(fd);
#endif
}

