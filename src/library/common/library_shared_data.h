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

#define _GNU_SOURCE            
#include <sched.h>


#include <common/config.h>
#include <common/types.h>
#include <common/states.h>
#include <common/types/signature.h>
#include <common/system/shared_areas.h>

typedef struct mpi_summary{
    float max;
    float min;
    float sd;
    float mean;
    float mag;
}mpi_summary_t;
/* Time is stored in USECS */
typedef struct mpi_information{
    unsigned int        total_mpi_calls;
    unsigned long long  exec_time;
    unsigned long long  mpi_time;
    int                 rank;
    double              perc_mpi;
}mpi_information_t;


typedef struct mpi_calls_types{
    ulong time_mpi;
    ulong time_period;
    ulong mpi;
    ulong sync;
    ulong collec;
    ulong blocking;
    ulong time_sync;
    ulong time_collec;
    ulong time_blocking;
    ulong sync_block;
    ulong time_sync_block;
    ulong max_sync_block;
}mpi_calls_types_t;


typedef struct lib_shared_data{
    uint 						earl_on;
    int             num_processes;
    unsigned int    num_signatures;
    double 			cas_counters;
    signature_t 	master_signature;
    int				master_rank;
    uint			master_ready;
}lib_shared_data_t;


typedef struct shsignature{
    uint 							master;
    pid_t 						pid;
    uint 							ready;
    uint 							iterations;
    mpi_information_t mpi_info;
    mpi_calls_types_t mpi_types_info;		
    /* sig was originally a signature_t */
    ssig_t 					sig;
    int 				  	app_state;
    unsigned long 	new_freq;
    cpu_set_t 			cpu_mask;
    int 						affinity;
    uint 						unbalanced;
    ulong 					policy_freq;
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
void print_sh_signature(int localid,shsignature_t *sig);
void clean_my_mpi_info(mpi_information_t *info);
void print_local_mpi_info(mpi_information_t *info);
void mpi_info_to_str(mpi_information_t *info,char *msg,size_t max);
void mpi_info_to_str_csv(mpi_information_t *info,char *msg,size_t max);
void mpi_info_head_to_str_csv(char *msg,size_t max);
void print_sig_readiness(lib_shared_data_t *data,shsignature_t *sig);




int select_cp(lib_shared_data_t *data,shsignature_t *sig);
int select_global_cp(int size,int max,int *ppn,shsignature_t *my_sh_sig,int *node_cp,int *rank_cp);
void compute_avg_sh_signatures(int size,int max,int *ppn,shsignature_t *my_sh_sig,signature_t *sig);

double min_perc_mpi_in_node(lib_shared_data_t *data,shsignature_t *sig);
void compute_per_node_avg_sig_info(lib_shared_data_t *data,shsignature_t *sig,shsignature_t *my_node_sig);


void copy_my_mpi_info(lib_shared_data_t *data,shsignature_t *sig,mpi_information_t *my_mpi_info);
void copy_my_sig_info(lib_shared_data_t *data,shsignature_t *sig,shsignature_t *rem_sig);
void shsignature_copy(shsignature_t *dst,shsignature_t *src);

int compute_per_node_most_loaded_process(lib_shared_data_t *data,shsignature_t *sig);
void compute_total_node_instructions(lib_shared_data_t *data,shsignature_t *sig,ull *t_inst);
void compute_total_node_flops(lib_shared_data_t *data,shsignature_t *sig,ull *flops);
void compute_total_node_cycles(lib_shared_data_t *data,shsignature_t *sig,ull *t_cycles);
void compute_total_io(lib_shared_data_t *data,shsignature_t *sig,ullong *total_io);
void compute_total_node_CPI(lib_shared_data_t *data,shsignature_t *sig,double *CPI);


uint compute_max_vpi(lib_shared_data_t *data,shsignature_t *sig,double *max_vpi);


double compute_node_flops(lib_shared_data_t *data,shsignature_t *sig);


void load_app_mgr_env();
int my_shsig_id();
int shsig_id(int node,int rank);

void* mpi_info_get_perc_mpi(void* mpi_inf);
#endif
