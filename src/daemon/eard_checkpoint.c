/**************************************************************
 * *   Energy Aware Runtime (EAR)
 * *   This program is part of the Energy Aware Runtime (EAR).
 * *
 * *   EAR provides a dynamic, transparent and ligth-weigth solution for
 * *   Energy management.
 * *
 * *       It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
 * *
 * *       Copyright (C) 2017  
 * *   BSC Contact     mailto:ear-support@bsc.es
 * *   Lenovo contact  mailto:hpchelp@lenovo.com
 * *
 * *   EAR is free software; you can redistribute it and/or
 * *   modify it under the terms of the GNU Lesser General Public
 * *   License as published by the Free Software Foundation; either
 * *   version 2.1 of the License, or (at your option) any later version.
 * *   
 * *   EAR is distributed in the hope that it will be useful,
 * *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 * *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * *   Lesser General Public License for more details.
 * *   
 * *   You should have received a copy of the GNU Lesser General Public
 * *   License along with EAR; if not, write to the Free Software
 * *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * *   The GNU LEsser General Public License is contained in the file COPYING  
 */
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/config.h>
#include <common/output/verbose.h>
#include <common/types/generic.h>
#include <daemon/eard_checkpoint.h>

extern char nodename[MAX_PATH_SIZE];
extern ulong eard_max_pstate;

void save_eard_conf(eard_dyn_conf_t *eard_dyn_conf)
{
#if 0
	char checkpoint_file[SZ_PATH];
	mode_t old_mask;
	int fd;
	if (eard_dyn_conf==NULL)	return;
	sprintf(checkpoint_file,"%s/%s",eard_dyn_conf->cconf->install.dir_temp,".eard_check");
	verbose(VCHCK,"Using checkpoint file %s",checkpoint_file);
	old_mask=umask(0);	
	fd=open(checkpoint_file,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR);
	if (fd<0){
		error("Error creating checkpoint file (%s)",strerror(errno));
		return;
	}	
	verbose(VCHCK,"saving node conf");
	// print_my_node_conf(eard_dyn_conf->nconf);
	// print_my_node_conf_fd_binary(fd,eard_dyn_conf->nconf);
	verbose(VCHCK,"saving current app");
	print_powermon_app_fd_binary(fd,eard_dyn_conf->pm_app);

	umask(old_mask);
	close(fd);
	#endif
}
void restore_eard_conf(eard_dyn_conf_t *eard_dyn_conf)
{
#if 0
	char checkpoint_file[SZ_PATH];
	int fd;
	if (eard_dyn_conf==NULL)	return;
	sprintf(checkpoint_file,"%s/%s",eard_dyn_conf->cconf->install.dir_temp,".eard_check");
	verbose(VCHCK,"Using checkpoint file %s",checkpoint_file);
    fd=open(checkpoint_file,O_RDONLY);
    if (fd<0){
        error("Error creating checkpoint file (%s)",strerror(errno));
        return;
    }
	read_my_node_conf_fd_binary(fd,eard_dyn_conf->nconf);	
	read_powermon_app_fd_binary(fd,eard_dyn_conf->pm_app);
	verbose(VCHCK,"restoring node conf");
	eard_max_pstate=eard_dyn_conf->nconf->max_pstate;
	verbose(VCHCK,"Node information recovered");
	print_my_node_conf(eard_dyn_conf->nconf);
	verbose(VCHCK,"Job recovered");
	report_job(&eard_dyn_conf->pm_app->app.job);
	close(fd);
#endif
}
