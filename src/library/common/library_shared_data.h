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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE            
#endif

#include <sched.h>

#include <common/types.h>
#include <common/states.h>
#include <common/config.h>
#include <common/types/signature.h>
#include <common/system/shared_areas.h>
#include <daemon/local_api/node_mgr.h>

typedef struct mpi_summary {
    float max;
    float min;
    float sd;
    float mean;
    float mag;
} mpi_summary_t;

typedef struct mpi_information {
    uint        total_mpi_calls;
    ullong      exec_time;       /**< Time stored in USECS. */
    ullong      mpi_time;
    int         rank;
    double      perc_mpi;
} mpi_information_t;

typedef struct mpi_calls_types
{
    ulong mpi_time;             // Time spent in MPI calls
    ulong exec_time;            // Execution time
    ulong mpi_call_cnt;         // Total MPI calls
    ulong mpi_sync_call_cnt;    // MPI synchronization calls count
    ulong mpi_collec_call_cnt;  // MPI collective calls count
    ulong mpi_block_call_cnt;   // MPI blocking calls count
    ulong mpi_sync_call_time;   // Time spent in MPI synchronization calls
    ulong mpi_collec_call_time; // Time spent in MPI collective calls
    ulong mpi_block_call_time;  // Time spent in MPI blocking calls
    ulong sync_block;
    ulong time_sync_block;
    ulong max_sync_block;
} mpi_calls_types_t;

typedef struct lib_shared_data {
    uint        earl_on;
    int         num_processes;
    uint        num_signatures;
    uint        num_cpus;
    ullong      cas_counters;
    signature_t node_signature;
    signature_t job_signature;
    int         master_rank;
    uint        master_ready;
    uint        global_stop_ready;
    cpu_set_t   node_mask;
    ulong       avg_cpufreq[MAX_CPUS_SUPPORTED]; // Average CPU freq for each processor of the job
    uint        node_mgr_index;
} lib_shared_data_t;


typedef struct shsignature {
    uint              master;
    pid_t             pid;
    uint              ready;
		uint 							exited;
    uint              iterations;
    mpi_information_t mpi_info;
    mpi_calls_types_t mpi_types_info;
    ssig_t            sig; // it was originally a signature_t
    int               app_state;
    ulong             new_freq;
	uint              num_cpus;
    cpu_set_t         cpu_mask;
    uint              pstate_index; /*!< The P-STATE selected by master for the CPUs associated, after policy_apply. */
    int               affinity;
    uint              unbalanced;
    // ulong             policy_freq;
	double            period;
} shsignature_t;

typedef struct node_mgr_sh_data {
	time_t            creation_time;
	int               fd_lib;
	int               fd_sig;
	lib_shared_data_t *libsh;
	shsignature_t     *shsig;
} node_mgr_sh_data_t;


/*********** SETTINGS configuration *******************/

/** Sets in path the filename for the shared memory area between MPI processes
 * @param path (output)
 */
int get_lib_shared_data_path(char *tmp, uint ID, char *path);

/** Creates the shared mmemory. It is used by node master process
*/
lib_shared_data_t * create_lib_shared_data_area(char * path);

/** Connects with a previously created shared memory region. It is used by no node master processes
*/
lib_shared_data_t * attach_lib_shared_data_area(char * path, int *fd);

/** Disconnect from a previously connected shared memory region. It is used by no node master processes
*/
void dettach_lib_shared_data_area(int fd);

/** Releases a shared memory area previously created. It is used by node master process
*/
void lib_shared_data_area_dispose(char * path);

/** Prints in the stderr values for the setting 
*/
void print_lib_shared_data(lib_shared_data_t *setting);


/************** SHARED SIGNATURES ***************/

/** Sets in path the filename for the shared memory area between MPI processes
 * @param path (output)
 */
int get_shared_signatures_path(char *tmp,uint ID, char *path);

/** Creates the shared mmemory. It is used by node master process
 */
shsignature_t * create_shared_signatures_area(char * path,int num_processes);

/** Connects with a previously created shared memory region. It is used by no node master processes
 */
shsignature_t * attach_shared_signatures_area(char * path,int num_processes, int *fd);

/** Disconnect from a previously connected shared memory region. It is used by no node master processes
 */
void dettach_shared_signatures_area(int fd);

/** Releases a shared memory area previously created. It is used by node master process
 */
void shared_signatures_area_dispose(char * path);

void signature_ready(shsignature_t *sig,int cur_state);
int compute_total_signatures_ready(lib_shared_data_t *data,shsignature_t *sig);

/** Marks all the sh signatures ready = 0 */
void free_node_signatures(lib_shared_data_t *data,shsignature_t *sig);

int all_signatures_initialized(lib_shared_data_t *data,shsignature_t *sig);
void aggregate_all_the_cpumasks(lib_shared_data_t *data,shsignature_t *sig, cpu_set_t *m);

int are_signatures_ready(lib_shared_data_t *data,shsignature_t *sig, uint *num_ready);
int num_processes_exited(lib_shared_data_t *data,shsignature_t *sig);

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
void compute_total_node_L1_misses(lib_shared_data_t *data,shsignature_t *sig,ull *t_L1);
void compute_total_node_L2_misses(lib_shared_data_t *data,shsignature_t *sig,ull *t_L2);
void compute_total_node_L3_misses(lib_shared_data_t *data,shsignature_t *sig,ull *t_L3);
void compute_total_node_cycles(lib_shared_data_t *data,shsignature_t *sig,ull *t_cycles);
void compute_total_io(lib_shared_data_t *data,shsignature_t *sig,ullong *total_io);
void compute_total_node_CPI(lib_shared_data_t *data,shsignature_t *sig,double *CPI);
void compute_total_node_avx_and_avx_fops(lib_shared_data_t *data,shsignature_t *sig,ullong *avx);
void compute_job_cpus(lib_shared_data_t *data,shsignature_t *sig,uint *cpus);

uint compute_max_vpi(lib_shared_data_t *data,shsignature_t *sig,double *max_vpi);

double compute_node_flops(lib_shared_data_t *data,shsignature_t *sig);

void load_app_mgr_env();
int my_shsig_id();
int shsig_id(int node,int rank);

void* mpi_info_get_perc_mpi(void *mpi_inf);
void* mpi_info_get_tcomp(void *mpi_inf);

/* Node mgr earl support functions */
void  init_earl_node_mgr_info();
void  update_earl_node_mgr_info();
#define PER_PROCESS 0
#define PER_JOB     1
void accum_estimations(lib_shared_data_t *data,shsignature_t *sig);
void estimate_power_and_gbs(lib_shared_data_t *data,shsignature_t *sig, node_mgr_sh_data_t *nmgr);
void verbose_jobs_in_node(int vl,ear_njob_t *nmgr_eard,node_mgr_sh_data_t *nmgr_earl);

#endif
