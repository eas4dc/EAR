/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
int  get_settings_conf_path(char *tmp, uint ID, char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/%u/.ear_settings_conf", tmp, ID);
	return EAR_SUCCESS;	
}
/** This functions creates the name of the file mapping the shared memory for the resched flag, it is placed at TMP 
 */
int  get_resched_path(char *tmp, uint ID,char *path)
{
	if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
	sprintf(path,"%s/%u/.ear_resched",tmp, ID);
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
settings_conf_t *create_settings_conf_shared_area(char * path, int *fd)  
{
	settings_conf_t my_settings;

	// Read and write permission for the owner. Read permission for the group and others.
	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	settings_conf_t *mem = (settings_conf_t *) create_shared_area(path, perms, (char *) &my_settings,
																																sizeof(my_settings), &fd_settings, 1);
	if (fd)
	{
		*fd = fd_settings;
	}

	return mem;
}


settings_conf_t * attach_settings_conf_shared_area(char * path)
{
    return (settings_conf_t *) attach_shared_area(path, sizeof(settings_conf_t), O_RDONLY, &fd_settings, NULL);
}


void dettach_settings_conf_shared_area()
{
	dettach_shared_area(fd_settings);
}


void settings_conf_shared_area_dispose(char *path, settings_conf_t *mem, int fd)
{
	if (mem != NULL) munmap(mem, sizeof(settings_conf_t));

	int fd_to_close = fd_settings;
	if (fd >= 0)
	{
		fd_to_close = fd;
	}
	
	dispose_shared_area(path, fd_to_close);
}


void print_settings_conf(settings_conf_t *setting)
{
	verbose(VCONF,"Settings\n--------\n\tUser type --- NORMAL(0)/AUTH(1)/ENERGY(2) --- %u\n"
            "\tLearning: %u\n\tLibrary enabled: %d\n"
            "\tPolicy --- min_energy(0)/min_time(1)/monitoring(2) --- %u\n",
            setting->user_type, setting->learning, setting->lib_enabled, setting->policy);
	verbose(VCONF,"\tMax. CPU freq.: %lu / Def. CPU freq.: %lu / Def. P-State: %u"
            " / CPU policy th.: %.2lf / CPU max. P-State: %d\n", setting->max_freq, setting->def_freq,
            setting->def_p_state, setting->settings[0], setting->cpu_max_pstate);

	print_earlib_conf(&setting->lib_info);

	verbose(VCONF,"\tMin. sign. power: %.0lf\n--------", setting->min_sig_power);
}


/// RESCHED

// Creates a shared memory region between eard and ear_lib. returns NULL if error.
resched_t *create_resched_shared_area(char *path, int *fd)  
{
	resched_t my_settings;
	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	resched_t *mem = (resched_t *) create_shared_area(path, perms, (char *) &my_settings,
																										sizeof(my_settings), &fd_resched, 1);

	if (fd != NULL)
	{
		*fd = fd_resched;
	}

	return mem;
}


resched_t * attach_resched_shared_area(char * path)
{
    return (resched_t *)attach_shared_area(path,sizeof(resched_t),O_RDWR,&fd_resched,NULL);
}


void dettach_resched_shared_area()
{
	dettach_shared_area(fd_resched);
}


void resched_shared_area_dispose(char * path, resched_t *mem, int fd)
{
	if (mem)
	{
		munmap(mem, sizeof(resched_t));
	}

	int fd_to_close = fd_resched;
	if (fd >= 0)
	{
		fd_to_close = fd;
	}

	dispose_shared_area(path, fd_to_close);
}


/************* APP_MGT
 *
 * Sets in path the filename for the shared memory area app_area between EARD and EARL
 * * @param path (output)
 * */
int get_app_mgt_path(char *tmp, uint ID,char *path)
{
  if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
  sprintf(path,"%s/%u/.ear_app_mgt",tmp,ID);
  return EAR_SUCCESS;
}


/** Creates the shared mmemory. It is used by EARD and APP. App puts information here
 * @param ear_conf_path specifies the path (folder) to create the file used by mmap. */
app_mgt_t * create_app_mgt_shared_area(char *path, int *fd)
{
  app_mgt_t my_app_data;
	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	app_mgt_t *mem;
  mem = (app_mgt_t *) create_shared_area(path, perms, (char *) &my_app_data,
																				 sizeof(my_app_data), &fd_app_mgt, 1);

	if (fd)
	{
		*fd = fd_app_mgt;
	}

	return mem;
}


/** Connects with a previously created shared memory region. It is used by EARLib (client)
 * @param ear_conf_path specifies the path (folder) where the mapped file were created. */
app_mgt_t * attach_app_mgt_shared_area(char *path)
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
void app_mgt_shared_area_dispose(char * path, app_mgt_t *mem, int fd)
{
	if (mem)
	{
		munmap(mem, sizeof(app_mgt_t));
	}

	int fd_to_close = (fd >= 0) ? fd : fd_app_mgt;
	dispose_shared_area(path, fd_to_close);
}


/****************************** PC_APP_INFO REGION *******************/
int get_pc_app_info_path(char *tmp,uint ID,char *path)
{
  if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
  sprintf(path,"%s/%u/.ear_pc_app_info",tmp, ID);
  return EAR_SUCCESS;
}


pc_app_info_t  *create_pc_app_info_shared_area(char *path, int *fd)
{
	pc_app_info_t my_data;
	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	pc_app_info_t *mem;
  mem = (pc_app_info_t *) create_shared_area(path, perms, (char *) &my_data,
																						 sizeof(my_data), &fd_pc_app_info, 1);

	if (fd)
	{
		*fd = fd_pc_app_info;
	}

	return mem;
}


pc_app_info_t * attach_pc_app_info_shared_area(char * path)
{
    return (pc_app_info_t *)attach_shared_area(path,sizeof(pc_app_info_t),O_RDWR,&fd_pc_app_info,NULL);
}


void dettach_pc_app_info_shared_area()
{
	dettach_shared_area(fd_pc_app_info);
}


void pc_app_info_shared_area_dispose(char * path, pc_app_info_t  *mem, int fd)
{
	if (mem)
	{
		munmap(mem, sizeof(pc_app_info_t));
	}

	int fd_to_close = (fd >= 0) ? fd : fd_pc_app_info;
	dispose_shared_area(path, fd_to_close);
}


/* COEFFS */


coefficient_t * create_coeffs_shared_area(char * path,coefficient_t *coeffs,int size)
{
	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	return (coefficient_t *) create_shared_area(path, perms, (char *) coeffs, size, &fd_coeffs, 0);
}


coefficient_t * create_coeffs_default_shared_area(char *path, coefficient_t *coeffs, int size)
{
	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	return (coefficient_t *) create_shared_area(path, perms, (char *) coeffs, size, &fd_coeffs, 0);
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


services_conf_t *create_services_conf_shared_area(char *path)
{
	services_conf_t my_services;
	if (path==NULL) return NULL;
	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  return (services_conf_t *) create_shared_area(path, perms, (char *) &my_services, sizeof(services_conf_t), &fd_services, 1);
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

	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	return (ulong *) create_shared_area(path, perms, (char *)f, size, &fd_freq, 0);
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


/******************** ear conf ****************************/
int get_ser_cluster_conf_path(char *tmp, char *path)
{
  if ((tmp==NULL) || (path==NULL)) return EAR_ERROR;
  sprintf(path,"%s/.cluster_conf",tmp);
  return EAR_SUCCESS;
}


int fd_cconf;
char  * create_ser_cluster_conf_shared_area(char *path, char *cconf, size_t size)
{
		mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    return (char *) create_shared_area(path, perms, (char *) cconf, size, &fd_cconf, 0);
}


char * attach_ser_cluster_conf_shared_area(char * path, size_t *size)
{
	    return (char *)attach_shared_area(path,0,O_RDONLY,&fd_cconf,(int *)size);
}


void dettach_ser_cluster_conf_shared_area()
{
	dettach_shared_area(fd_cconf);
}


void cluster_ser_conf_shared_area_dispose(char * path)
{
	if (path==NULL) return;
  dispose_shared_area(path,fd_cconf);

}
