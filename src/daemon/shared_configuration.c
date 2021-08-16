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
#include <common/output/verbose.h>
#include <common/types/coefficient.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/configuration/cluster_conf_earlib.h>
#include <daemon/shared_configuration.h>

static int fd_settings,fd_resched,fd_coeffs,fd_services,fd_freq;
static int fd_app_mgt,fd_pc_app_info;

/** These functions created path names, just to avoid problems if changing the path name in the future */
/** This functions creates the name of the file mapping the shared memory for the dynamic power settings, it is placed at TMP 
 */
int  get_settings_conf_path(char *tmp,char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/.ear_settings_conf",tmp);
	return EAR_SUCCESS;	
}
/** This functions creates the name of the file mapping the shared memory for the resched flag, it is placed at TMP 
 */
int  get_resched_path(char *tmp,char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/.ear_resched",tmp);
	return EAR_SUCCESS;	
}

int get_coeffs_path(char *tmp,char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/.ear_coeffs",tmp);
	return EAR_SUCCESS;	
}

int get_coeffs_default_path(char *tmp,char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
  sprintf(path,"%s/.ear_coeffs_default",tmp);
	return EAR_SUCCESS;	
}




//// SETTINGS

// Creates a shared memory region between eard and ear_lib. returns NULL if error.
settings_conf_t * create_settings_conf_shared_area(char * path)  
{      	
	settings_conf_t my_settings;
	
	return (settings_conf_t *)create_shared_area(path,(char *)&my_settings,sizeof(my_settings),&fd_settings,1);
	
}

settings_conf_t * attach_settings_conf_shared_area(char * path)
{
    return (settings_conf_t *)attach_shared_area(path,sizeof(settings_conf_t),O_RDONLY,&fd_settings,NULL);
}                                
void dettach_settings_conf_shared_area()
{
	dettach_shared_area(fd_settings);
}

void settings_conf_shared_area_dispose(char * path)
{
	dispose_shared_area(path,fd_settings);
}


void print_settings_conf(settings_conf_t *setting)
{
	verbose(VCONF,"settings: user_type(0=NORMAL,1=AUTH,2=ENERGY) %u learning %u lib_enabled %d policy(0=min_energy, 1=min_time,2=monitoring) %u \n",
	setting->user_type,setting->learning,setting->lib_enabled,setting->policy);
	verbose(VCONF,"\tmax_freq %lu def_freq %lu def_p_state %u th %.2lf\n",setting->max_freq,setting->def_freq,setting->def_p_state,setting->settings[0]);
	print_earlib_conf(&setting->lib_info);	
	verbose(VCONF,"\tmin_sig_power %.0lf",setting->min_sig_power);

}

/// RESCHED

// Creates a shared memory region between eard and ear_lib. returns NULL if error.
resched_t * create_resched_shared_area(char * path)  
{      	
	resched_t my_settings;
	
	return (resched_t *)create_shared_area(path,(char *)&my_settings,sizeof(my_settings),&fd_resched,1);
	
}

resched_t * attach_resched_shared_area(char * path)
{
    return (resched_t *)attach_shared_area(path,sizeof(resched_t),O_RDWR,&fd_resched,NULL);
}                                
void dettach_resched_shared_area()
{
	dettach_shared_area(fd_resched);
}
void resched_shared_area_dispose(char * path)
{
	dispose_shared_area(path,fd_resched);
}


#if POWERCAP
/************* APP_MGT
 *
 * Sets in path the filename for the shared memory area app_area between EARD and EARL
 * * @param path (output)
 * */
int get_app_mgt_path(char *tmp,char *path)
{
  if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
  sprintf(path,"%s/.ear_app_mgt",tmp);
  return EAR_SUCCESS;
}

/** Creates the shared mmemory. It is used by EARD and APP. App puts information here
 *  *  * *   @param ear_conf_path specifies the path (folder) to create the file used by mmap
 *   *   * */

app_mgt_t * create_app_mgt_shared_area(char *path)
{
  app_mgt_t my_app_data;

  return (app_mgt_t *)create_shared_area(path,(char *)&my_app_data,sizeof(my_app_data),&fd_app_mgt,1);

}

/** Connects with a previously created shared memory region. It is used by EARLib (client)
 *  *  * *   @param ear_conf_path specifies the path (folder) where the mapped file were created
 *   *   * */
app_mgt_t * attach_app_mgt_shared_area(char * path)
{
    return (app_mgt_t *)attach_shared_area(path,sizeof(app_mgt_t),O_RDWR,&fd_app_mgt,NULL);

}

/** Disconnect from a previously connected shared memory region. It is used by EARLib (client)
 *  *  * */
void dettach_app_mgt_shared_area()
{
	dettach_shared_area(fd_app_mgt);
}

/** Releases a shared memory area previously created. It is used by EARD (server)
 *  *  * */
void app_mgt_shared_area_dispose(char * path)
{
	dispose_shared_area(path,fd_app_mgt);
}

/****************************** PC_APP_INFO REGION *******************/
int get_pc_app_info_path(char *tmp,char *path)
{
  if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
  sprintf(path,"%s/.ear_pc_app_info",tmp);
  return EAR_SUCCESS;
}
pc_app_info_t  * create_pc_app_info_shared_area(char *path)
{
	pc_app_info_t my_data;
  return (pc_app_info_t *)create_shared_area(path,(char *)&my_data,sizeof(my_data),&fd_pc_app_info,1);

}
pc_app_info_t * attach_pc_app_info_shared_area(char * path)
{
    return (pc_app_info_t *)attach_shared_area(path,sizeof(pc_app_info_t),O_RDWR,&fd_pc_app_info,NULL);
}
void dettach_pc_app_info_shared_area()
{
	dettach_shared_area(fd_pc_app_info);
}
void pc_app_info_shared_area_dispose(char * path)
{
	dispose_shared_area(path,fd_pc_app_info);
}

#endif


/* COEFFS */

coefficient_t * create_coeffs_shared_area(char * path,coefficient_t *coeffs,int size)
{
	return (coefficient_t *)create_shared_area(path,(char *)coeffs,size,&fd_coeffs,0);

}

coefficient_t * create_coeffs_default_shared_area(char * path,coefficient_t *coeffs,int size)
{

    return (coefficient_t *)create_shared_area(path,(char *)coeffs,size,&fd_coeffs,0);

}


coefficient_t * attach_coeffs_shared_area(char * path,int *size)
{
    return (coefficient_t *)attach_shared_area(path,0,O_RDONLY,&fd_coeffs,size);
}
coefficient_t * attach_coeffs_default_shared_area(char * path,int *size)
{
    return (coefficient_t *)attach_shared_area(path,0,O_RDONLY,&fd_coeffs,size);
}


void coeffs_shared_area_dispose(char * path)
{
    dispose_shared_area(path,fd_coeffs);
}

void coeffs_default_shared_area_dispose(char * path)
{
    dispose_shared_area(path,fd_coeffs);
}


void dettach_coeffs_shared_area()
{
    dettach_shared_area(fd_coeffs);
}

void dettach_coeffs_default_shared_area()
{
    dettach_shared_area(fd_coeffs);
}




/**** SERVICES **********/
int get_services_conf_path(char *tmp,char *path)
{
		if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
    sprintf(path,"%s/.ear_services_conf",tmp);
		return EAR_SUCCESS;
}

services_conf_t * create_services_conf_shared_area(char * path)
{
	services_conf_t my_services;
	if (path==NULL) return NULL;
  return (services_conf_t *)create_shared_area(path,(char *)&my_services,sizeof(services_conf_t),&fd_services,1);
}

services_conf_t * attach_services_conf_shared_area(char * path)
{
	if (path==NULL) return NULL;
  return (services_conf_t *)attach_shared_area(path,sizeof(services_conf_t),O_RDONLY,&fd_services,NULL);
}

void services_conf_shared_area_dispose(char * path)
{
	if (path==NULL) return;
  dispose_shared_area(path,fd_services);
}

void dettach_services_conf_shared_area()
{
    dettach_shared_area(fd_services);
}

/*************** FREQUENCIES ****************/
int get_frequencies_path(char *tmp,char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/.ear_frequencies",tmp);
	return EAR_SUCCESS;
}
ulong *create_frequencies_shared_area(char * path,ulong *f,int size)
{
	if ((path==NULL)|| (f==NULL)) return NULL;
	return (ulong *)create_shared_area(path,(char *)f,size,&fd_freq,0);
}
ulong *attach_frequencies_shared_area(char * path,int *size)
{
	if ((path==NULL) || (size==NULL)) return NULL;
	return (ulong *)attach_shared_area(path,0,O_RDONLY,&fd_freq,size);
}
void frequencies_shared_area_dispose(char * path)
{
	if (path==NULL) return;
	dispose_shared_area(path,fd_freq);
}
void dettach_frequencies_shared_area()
{
	dettach_shared_area(fd_freq);
}

