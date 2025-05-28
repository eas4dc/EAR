/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/**
 *    \file shared_configuration.h
 *    \brief This file defines the API to create/attach/dettach/release the shared memory area
 *    between the EARD and the EARLib. It is used to communicate the EARLib updates remotelly
 *    requested.
 *
 *    It also defines structures to be shared among processes running concurrently the Library.
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

/** \name MPI data */
/**@{*/
#if SHARED_MPI_SUMMARY
/** Used to share information between master ranks. */
typedef struct mpi_summary {
    float max;
    float min;
    float sd;
    float mean;
    float mag;
} mpi_summary_t;
#endif


/** Basic MPI statistics. Granularity: process. */
typedef struct mpi_information {
    ullong      total_mpi_calls; /**< The number of MPI calls. */
    ullong      exec_time;       /**< Elapsed time, in microseconds. */
    ullong      mpi_time;        /**< Time spent in MPI calls, in microseconds. */
    int         rank;            /**< The process rank. */
    double      perc_mpi;        /**< Percentage of time spent in MPI calls, i.e., mpi_time / exec_time. */
} mpi_information_t;


/** MPI call types statistics. Granularity: process. */
typedef struct mpi_calls_types
{
    ulong mpi_time;             /**< Time spent in MPI calls. */
    ulong exec_time;            /**< Execution time. */
    ulong mpi_call_cnt;         /**< Total MPI calls. */
    ulong mpi_sync_call_cnt;    /**< MPI synchronization calls count. */
    ulong mpi_collec_call_cnt;  /**< MPI collective calls count. */
    ulong mpi_block_call_cnt;   /**< MPI blocking calls count. */
    ulong mpi_sync_call_time;   /**< Time spent in MPI synchronization calls.*/ 
    ulong mpi_collec_call_time; /**< Time spent in MPI collective calls. */
    ulong mpi_block_call_time;  /**< Time spent in MPI blocking calls. */
    ulong max_sync_block;
} mpi_calls_types_t;
/**@}*/


#define MASTER    (masters_info.my_master_rank >= 0)
#define MASTER_ID masters_info.my_master_rank
/** \name Shared info
 * This group of definitions is used to share data between processes. */
/**@{*/

/** Global application data shared between processes. */
typedef struct lib_shared_data {
    uint        earl_on;
    int         num_processes;
    uint        num_signatures;
    uint        num_cpus;													/**< Number of CPUs used for the job. Computed from node_mask attribute. */
    float       total_cache_bwidth;
    ullong      cas_counters;
    signature_t node_signature;
    signature_t job_signature;
    int         master_rank;
    uint        master_ready;
    uint        global_stop_ready;
    cpu_set_t   node_mask;											 /**< The CPU_SET mask of the job in the node. */
    ulong       avg_cpufreq[MAX_CPUS_SUPPORTED]; /**< Average CPU freq for each processor of the job. */
    uint        node_mgr_index;
		uint        reduced_in_barrier;							 /**< This variable is only used in an unstable/research policy. */
		uint        estimate_node_metrics;
#if MPI_OPTIMIZED
    uint    processes_in_barrier;
#endif
} lib_shared_data_t;


/** Per-process application data. */
typedef struct shsignature {
	uint              master;
	pid_t             pid;
	uint              ready;
	uint              exited;
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
	float             perc_MPI;
	double            period;
#if MPI_OPTIMIZED
	ulong             mpi_freq;
#endif
	uint cpu_util; /*!< The CPU utilization computed from Proc Stat */
} shsignature_t;


typedef struct node_mgr_sh_data {
  job_id            jid;
  job_id            sid;
  job_id            lid;
	time_t            creation_time;
	time_t            modification_time;
	int               fd_lib;
	int               fd_sig;
	lib_shared_data_t *libsh;
	shsignature_t     *shsig;
} node_mgr_sh_data_t;
/**@}*/


void eid_folder(char *dst, int size, char *tmp, int jid, int sid, uint aid);


/*********** SETTINGS configuration *******************/

/** Sets in path the filename for the shared memory area between MPI processes
 * @param[out] path Output path. */
int get_lib_shared_data_path(char *tmp, uint ID, uint AID, char *path);


/** \brief Creates the shared memory.
 * It is used by node master process. */
lib_shared_data_t * create_lib_shared_data_area(char * path);


/** \brief Connects with a previously created shared memory region.
 * It is used by no node master processes. */
lib_shared_data_t * attach_lib_shared_data_area(char * path, int *fd);


/** \brief Disconnect from a previously connected shared memory region.
 * It is used by no node master processes. */
void dettach_lib_shared_data_area(int fd);


/** \brief Releases a shared memory area previously created.
 * It is used by node master process. */
void lib_shared_data_area_dispose(char *path);


/** Prints in the stderr values for the setting. */
void print_lib_shared_data(lib_shared_data_t *setting);

/************** SHARED SIGNATURES ***************/

/** Sets in path the filename for the shared memory area between MPI processes
 * @param path (output) */
int get_shared_signatures_path(char *tmp,uint ID, uint AID, char *path);


/** Creates the shared mmemory.
 * It is used by node master process. */
shsignature_t * create_shared_signatures_area(char * path, int num_processes);


/** Connects with a previously created shared memory region.
 * It is used by no node master processes. */
shsignature_t * attach_shared_signatures_area(char * path,int num_processes, int *fd);


/** Disconnect from a previously connected shared memory region.
 * It is used by no node master processes. */
void dettach_shared_signatures_area(int fd);


/** Releases a shared memory area previously created.
 * It is used by node master process. */
void shared_signatures_area_dispose(char * path);


void signature_ready(shsignature_t *sig, int cur_state);


int compute_total_signatures_ready(lib_shared_data_t *data, shsignature_t *sig);


/** Marks all the sh signatures ready = 0 */
void free_node_signatures(lib_shared_data_t *data, shsignature_t *sig);


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

/** Computes the total number of instructions of a job where its \p n_procs processses have their signatures
 * at \p sig. The result is stored at the address pointed by \p instructions. */
state_t compute_job_node_instructions(const shsignature_t *sig, int n_procs, ull *instructions);


/** Computes the total number of FLOP instructions of a job where its \p n_procs
 * processses have their signatures at \p sig. The result is stored at the
 * address pointed by \p flops.
 *
 * \param flops The address of this pointer must be the first position of an
 * allocated space of size FLOPS_EVENTS * sizeof (unsigned long). Otherwise
 * you will overwrite memory. */
state_t compute_job_node_flops(const shsignature_t *sig, int n_procs, ull *flops);

/** Computes the accumulated IO MB_S */
state_t compute_job_node_io_mbs(const shsignature_t *sig, int n_procs, double *io_mbs);

/** Computes the total number of cycles of a job where its \p n_procs
 * processses have their signatures at \p sig. The result is stored
 * at the address pointed by \p t_cycles. */
state_t compute_job_node_cycles(const shsignature_t *sig, int n_procs, ull *t_cycles);

/** Computes the total number of L1 misses of a job where its \p n_procs
 * processses have their signatures at \p sig. The result is stored at
 * the address pointed by \p t_L1. */
state_t compute_job_node_L1_misses(const shsignature_t *sig, int n_procs, ull *t_L1);

/** Computes the total number of L2 misses of a job where its \p n_procs processses have their
 * signatures at \p sig. The result is stored at the address pointed by \p t_L2. */
state_t compute_job_node_L2_misses(const shsignature_t *sig, int n_procs, ull *t_L2);

/** Computes the total number of L3 misses of a job where its \p n_procs processses have their
 * signatures at \p sig. The result is stored at the address pointed by \p t_L3. */
state_t compute_job_node_L3_misses(const shsignature_t *sig, int n_procs, ull *t_L3);

/** Compute the total GFlop/s rate of a job where its \p n_procs processes have their signatures
 * at \p sig. The result is stored at the address pointed by \p t_gflops. */
state_t compute_job_node_gflops(const shsignature_t *sig, int n_procs, double *t_gflops);

uint compute_max_vpi_idx(const shsignature_t *sig, int n_procs, double *max_vpi);

void compute_total_io(lib_shared_data_t *data,shsignature_t *sig,ullong *total_io);
void compute_total_node_avx_and_avx_fops(lib_shared_data_t *data,shsignature_t *sig,ullong *avx);

/** Counts the number of CPUs of the job (on the current node) based on 
 * \p data's node_mask attribute and stores it on \p cpus. */
void compute_job_cpus(lib_shared_data_t *data, uint *cpus);

state_t compute_job_node_cpuutil(const shsignature_t *sig, int n_procs, uint *t_cpuutil);

void load_app_mgr_env();
int my_shsig_id();
int shsig_id(int node,int rank);

void* mpi_info_get_perc_mpi(void *mpi_inf);

/* Node mgr earl support functions */
void init_earl_node_mgr_info();
void update_earl_node_mgr_info();
void release_earl_node_mgr_info();

/** Returns how many apps are running in the node */
uint node_mgr_earl_apps();
#define PER_PROCESS 0
#define PER_JOB     1
void accum_estimations(lib_shared_data_t *data,shsignature_t *sig);
void estimate_power_and_gbs(lib_shared_data_t *data,shsignature_t *sig, node_mgr_sh_data_t *nmgr);
void verbose_jobs_in_node(int vl,ear_njob_t *nmgr_eard,node_mgr_sh_data_t *nmgr_earl);

/** Updates all affinity masks of shared signatures array pointed by \p sig.
 * This function also updates the job mask of the lib_shared_data pointed by \p data.
 * Returns EAR_ERROR with the appropiate messages either when the input arguments are NULL
 * or when sched_getafinity call returns an error value, with its corresponding errno value.
 * On error, all modified masks are restored to ones passed within arguments. */
state_t update_job_affinity_mask(lib_shared_data_t *data, shsignature_t *sig);

#endif // _LIB_SHARED_DATA_H
