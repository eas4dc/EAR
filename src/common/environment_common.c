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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/config/config_env.h>

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
	my_ear_tmp=ear_getenv(ENV_PATH_TMP);
	if (my_ear_tmp==NULL){
		my_ear_tmp=ear_getenv("TMP");
		//??
		if (my_ear_tmp==NULL) my_ear_tmp=ear_getenv("HOME");
	}
	conf_ear_tmp=malloc(strlen(my_ear_tmp)+1);
	strcpy(conf_ear_tmp,my_ear_tmp);
	return conf_ear_tmp;	
}

int getenv_ear_verbose()
{
	char *my_verbose;
	my_verbose=ear_getenv(ENV_FLAG_VERBOSITY);
	if (my_verbose!=NULL){
		conf_ear_verbose=atoi(my_verbose);
		if ((conf_ear_verbose<0) || (conf_ear_verbose>4)) conf_ear_verbose=DEFAULT_VERBOSE;
	}	
	return conf_ear_verbose;
}

char *get_ear_install_path()
{
	return ear_getenv(ENV_PATH_EAR);
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

// Is here because environment.c is full of things like dlsym().
char *ear_getenv(const char *name)
{
    static char *pfx[] = { "", "SCHED_", "SLURM_", "OAR_", "PBS_"};
    static int pfx_n = 5;
    char buffer[128];
    char *var;
    int i;

    for (i = 0; i < pfx_n; ++i) {
        // clean and concat
        buffer[0] = '\0';
        strcat(buffer, pfx[i]);
        strcat(buffer, name);
        if ((var = getenv((const char *) buffer)) != NULL) {
            return var;
        }
    }
    return NULL;
}
