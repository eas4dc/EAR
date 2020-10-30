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

#ifndef _LIB_SHARED_DATA_H
#define _LIB_SHARED_DATA_H

#include <common/system/shared_areas.h>
#include <common/config.h>
#include <common/types/signature.h>

typedef struct lib_shared_data{
	int num_processes;
	unsigned int num_signatures;
	double cas_counters;
}lib_shared_data_t;

typedef struct mpi_information{
	unsigned int total_mpi_calls;
	unsigned long long exec_time;
	unsigned long long mpi_time;
	int rank;
	double perc_mpi;
}mpi_information_t;

typedef struct shsignature{
	uint master;
	uint ready;
	mpi_information_t mpi_info;
	signature_t sig;
	int 				  app_state;
	unsigned long new_freq;
}shsignature_t;


/*********** SETTINGS configuration *******************/

/** Sets in path the filename for the shared memory area between MPI processes
 * @param path (output)
 */
int get_lib_shared_data_path(char *tmp,char *path);

/** Creates the shared mmemory. It is used by node master process
*/

lib_shared_data_t * create_lib_shared_data_area(char * path);

/** Connects with a previously created shared memory region. It is used by no node master processes
*/
lib_shared_data_t * attach_lib_shared_data_area(char * path);

/** Disconnect from a previously connected shared memory region. It is used by no node master processes
*/
void dettach_lib_shared_data_area();

/** Releases a shared memory area previously created. It is used by node master process
*/
void lib_shared_data_area_dispose(char * path);

/** Prints in the stderr values for the setting 
*/
void print_lib_shared_data(lib_shared_data_t *setting);


/************** SHARED SIGNATURES ***************/

/** Sets in path the filename for the shared memory area between MPI processes
 *  * @param path (output)
 *   */
int get_shared_signatures_path(char *tmp,char *path);

/** Creates the shared mmemory. It is used by node master process
 * */

shsignature_t * create_shared_signatures_area(char * path,int num_processes);

/** Connects with a previously created shared memory region. It is used by no node master processes
 * */
shsignature_t * attach_shared_signatures_area(char * path,int num_processes);

/** Disconnect from a previously connected shared memory region. It is used by no node master processes
 * */
void dettach_shared_signatures_area();

/** Releases a shared memory area previously created. It is used by node master process
 * */
void shared_signatures_area_dispose(char * path);


void signature_ready(shsignature_t *sig,int cur_state);
int compute_total_signatures_ready(lib_shared_data_t *data,shsignature_t *sig);
int are_signatures_ready(lib_shared_data_t *data,shsignature_t *sig);
void clean_signatures(lib_shared_data_t *data,shsignature_t *sig);
void clean_mpi_info(lib_shared_data_t *data,shsignature_t *sig);
void print_shared_signatures(lib_shared_data_t *data,shsignature_t *sig);
void print_sh_signature(shsignature_t *sig);
void clean_my_mpi_info(mpi_information_t *info);
void print_local_mpi_info(mpi_information_t *info);

int select_cp(lib_shared_data_t *data,shsignature_t *sig);
int select_global_cp(int size,int max,int *ppn,shsignature_t *my_sh_sig,int *node_cp,int *rank_cp);
double min_perc_mpi_in_node(lib_shared_data_t *data,shsignature_t *sig);

void copy_my_mpi_info(lib_shared_data_t *data,shsignature_t *sig,mpi_information_t *my_mpi_info);
void copy_my_sig_info(lib_shared_data_t *data,shsignature_t *sig,shsignature_t *rem_sig);

#endif
