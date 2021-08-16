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

/**
*    \file shared_configuration.h
*    \brief This file defines the API to create/attach/dettach/release the shared memory area between the EARD and the EARLib. It is used to communicate the EARLib updates 
*	remotelly requested
*/

#ifndef _SHARED_CONF_H
#define _SHARED_CONF_H

#include <common/config.h>
#include <common/system/shared_areas.h>
#include <common/types/generic.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/types/coefficient.h>

#ifdef POWERCAP
#include <daemon/powercap/powercap.h>
#include <daemon/app_mgt.h>
#include <common/types/pc_app_info.h>
#endif

typedef struct services_conf{
		eard_conf_t     eard;
    eargm_conf_t    eargmd;
		db_conf_t 		db;
    eardb_conf_t 	eardbd;
		char net_ext[ID_SIZE];
}services_conf_t;

typedef struct settings_conf{
	uint 	id;
	uint 	user_type;
	uint 	learning;
	uint 	lib_enabled;
	uint 	policy;
	ulong 	max_freq;
	ulong	def_freq;
	uint	def_p_state;
	double 	settings[MAX_POLICY_SETTINGS];
  char    policy_name[64];
	earlib_conf_t lib_info;
	double min_sig_power;
	double max_sig_power;
	double max_power_cap;
	uint report_loops;
	conf_install_t 	installation;
	ulong   max_avx512_freq;
  ulong   max_avx2_freq;
#if POWERCAP
	node_powercap_opt_t pc_opt;
#endif
} settings_conf_t;



typedef struct resched{
	int 	force_rescheduling;
}resched_t;

/*********** SETTINGS configuration *******************/

/** Sets in path the filename for the shared memory area between EARD and EARL
 * @param path (output)
 */
int get_settings_conf_path(char *tmp,char *path);

/** Creates the shared mmemory. It is used by EARD (server)
*	@param ear_conf_path specifies the path (folder) to create the file used by mmap
*/

settings_conf_t * create_settings_conf_shared_area(char * path);

/** Connects with a previously created shared memory region. It is used by EARLib (client)
*	@param ear_conf_path specifies the path (folder) where the mapped file were created
*/
settings_conf_t * attach_settings_conf_shared_area(char * path);

/** Disconnect from a previously connected shared memory region. It is used by EARLib (client)
*/
void dettach_settings_conf_shared_area();

/** Releases a shared memory area previously created. It is used by EARD (server)
*/
void settings_conf_shared_area_dispose(char * path);

/** Prints in the stderr values for the setting 
*/
void print_settings_conf(settings_conf_t *setting);

/*********** RESCHED ****************************/
/** Sets in path the filename for the shared memory area between EARD and EARL
 *  * @param path (output)
 *   */
int get_resched_path(char *tmp,char *path);

/** Creates the shared mmemory. It is used by EARD (server)
 * *   @param ear_conf_path specifies the path (folder) to create the file used by mmap
 * */

resched_t * create_resched_shared_area(char *path);

/** Connects with a previously created shared memory region. It is used by EARLib (client)
 * *   @param ear_conf_path specifies the path (folder) where the mapped file were created
 * */
resched_t * attach_resched_shared_area(char * path);

/** Disconnect from a previously connected shared memory region. It is used by EARLib (client)
 * */
void dettach_resched_shared_area();

/** Releases a shared memory area previously created. It is used by EARD (server)
 * */
void resched_shared_area_dispose(char * path);

#if POWERCAP
/****************** APP AREA ***************/

/** Sets in path the filename for the shared memory area app_area between EARD and EARL
* @param path (output)
*/
int get_app_mgt_path(char *tmp,char *path);

/** Creates the shared mmemory. It is used by EARD and APP. App puts information here
 *  * *   @param ear_conf_path specifies the path (folder) to create the file used by mmap
 *   * */

app_mgt_t * create_app_mgt_shared_area(char *path);

/** Connects with a previously created shared memory region. It is used by EARLib (client)
 *  * *   @param ear_conf_path specifies the path (folder) where the mapped file were created
 *   * */
app_mgt_t * attach_app_mgt_shared_area(char * path);

/** Disconnect from a previously connected shared memory region. It is used by EARLib (client)
 *  * */
void dettach_app_mgt_shared_area();

/** Releases a shared memory area previously created. It is used by EARD (server)
 *  * */
void app_mgt_shared_area_dispose(char * path);
#endif


/***************** COEFFICIENTS **********/
int get_coeffs_path(char *tmp,char *path);

coefficient_t * create_coeffs_shared_area(char * path,coefficient_t *coeffs,int size);

coefficient_t * attach_coeffs_shared_area(char * path,int *size);

void coeffs_shared_area_dispose(char * path);

void dettach_coeffs_shared_area();

int get_coeffs_default_path(char *tmp,char *path);

coefficient_t * create_coeffs_default_shared_area(char * path,coefficient_t *coeffs,int size);

coefficient_t * attach_coeffs_default_shared_area(char * path,int *size);

void coeffs_default_shared_area_dispose(char * path);

void dettach_coeffs_default_shared_area();


/*** SERVICES ***/

/** Sets in path the path for the services configuration in the shared memory */
int get_services_conf_path(char *tmp,char *path);

/** Creates and maps the shared memory region for services, to be used by eard. path if created using get_services_conf_path */
services_conf_t * create_services_conf_shared_area(char * path);

/** Maps the shared memory region for services, to be used by ear plugin or any other component.path if created using get_services_conf_path */
services_conf_t * attach_services_conf_shared_area(char * path);

/** Releases the shared memory for services_conf, to be used by eard */
void services_conf_shared_area_dispose(char * path);

/** Unmmaps the shared memory for services_conf, to be used by ear plugin or any other component */
void dettach_services_conf_shared_area();


/************** FREQUENCIES ***************/

/** Sets un path the path fort the list of frequencies in the shared memory */
int get_frequencies_path(char *tmp,char *path);

/** Creates and maps the shared memory region for list of frequencies. size is the input size */
ulong *create_frequencies_shared_area(char * path,ulong *f,int size);

/** Maps the shared memory region for list of frequencies. size is vector size in butes, num_pstates=size/sizeof(ulong) */
ulong *attach_frequencies_shared_area(char * path,int *size);

/** Releases the shared memory for the list of frequencies */
void frequencies_shared_area_dispose(char * path);

/** Unmmaps the shared memory for the list of frequencies */
void dettach_frequencies_shared_area();

#if POWERCAP
/************** PC_APP_INFO_T ****************/
int get_pc_app_info_path(char *tmp,char *path);
pc_app_info_t  * create_pc_app_info_shared_area(char *path);
pc_app_info_t * attach_pc_app_info_shared_area(char * path);
void dettach_pc_app_info_shared_area();
void pc_app_info_shared_area_dispose(char * path);
#endif



#endif
