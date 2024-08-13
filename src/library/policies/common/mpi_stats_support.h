/*
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* 
* BSC Contact   mailto:ear-support@bsc.es
* 
*
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef MPI_SUPPORT_H
#define MPI_SUPPORT_H

/** This file provides utility functions to work with MPI data. */

#include <library/api/mpi_support.h>
#include <library/policies/policy_ctx.h>
#include <library/common/library_shared_data.h>
#include <common/system/execute.h>

#if MPI_OPTIMIZED
#include <common/system/lock.h>
extern pthread_mutex_t ear_mpi_opt_lock;
#endif

/** Called on MPI_Init */
state_t mpi_app_init(polctx_t *c);
/** Called on MPI_Finalize */
state_t mpi_app_end(polctx_t *c);
/** Called at each MPI call */
state_t mpi_call_init(polctx_t *c, mpi_call call_type);
/** Called once a MPI call just ended */
state_t mpi_call_end(polctx_t *c, mpi_call call_type);

void monitor_mpi_calls(uint on);
float avg_mpi_calls_per_second();
unsigned long long int total_mpi_call_statistics();
uint is_monitor_mpi_calls_enabled();
uint is_mpi_intensive();
uint is_low_mpi_activity();

/** Per-Node Executes the read-diff for mpi statistics on mpi calls types */
state_t read_diff_node_mpi_types_info(lib_shared_data_t *data, shsignature_t *sig,
        mpi_calls_types_t *node_mpi_calls, mpi_calls_types_t *last_node);
/* Per-Node Executes the read-diff for mpi statistics */
state_t read_diff_node_mpi_info(lib_shared_data_t *data, shsignature_t *sig,
        mpi_information_t *node_mpi_calls, mpi_information_t *last_node);

state_t is_blocking_busy_waiting(uint *block_type);
uint is_blocking_collective(mpi_call call);
uint is_bgather(mpi_call call);
uint is_bbarrier(mpi_call call);
uint is_bwait(mpi_call call);



/** \defgroup mpi_verbose Verbose
 * @{
 */
void verbose_mpi_calls_types(int verb_lvl, mpi_calls_types_t *t);
void verbose_mpi_data(int verb_lvl, mpi_information_t *mc);
state_t mpi_support_verbose_perc_mpi_stats(int verb_lvl, double *percs_mpi, int num_procs,
        double mean, double median, double sd, double mag);
/** @} */

/** Based on perc. of mpi calls for each of \p num_procs passed, compute basic statistics
 * (therefore stores their results) and determines whether the application is load balanced. */
state_t mpi_support_evaluate_lb(const mpi_information_t *node_mpi_calls, int num_procs,
        double *percs_mpi, double *mean, double *sd, double *mag, uint *lb);
/** This function sets the critical path and also stores max and min perc_mpi processes. */
state_t mpi_support_select_critical_path(uint *critical_path, double *percs_mpi, int num_procs,
        double mean, double *median, int *max_mpi, int *min_mpi);
/** This function checks whether current mpi data has changed from previous iteration. */
state_t mpi_support_mpi_changed(double current_mag, double prev_mag, uint *cp, uint *prev_cp,
        int num_procs, double *similarity, uint *mpi_changed);

/** \defgroup mpi_stats MPI statistics
 * These group of pure functions provide utilities to compute MPI statistics.
 * @{
 */
state_t mpi_stats_evaluate_similarity(double *current_perc_mpi, double *last_perc_mpi,
        size_t size, double *similarity);
/**@}*/

#if MPI_OPTIMIZED
state_t must_be_optimized(mpi_call call_type, p2i buf, p2i dest, ulong *elapsed);
#endif
state_t already_optimized(mpi_call call_type, p2i buf, p2i dest);

/** Copies MPI metrics from \p src to \p dst.
 * If `src` is a NULL pointer, `dst` is filled with the MPI metrics of the `i`-th process
 * (local rank) stored in the shared region.
 * If `i` is negative, `dst` is filled with the MPI metrics of the current process (my_node_id). */
state_t mpi_call_get_stats(mpi_information_t *dst, mpi_information_t * src, int i);
state_t mpi_call_get_types(mpi_calls_types_t *dst, shsignature_t * src, int i);
state_t mpi_call_types_diff(mpi_calls_types_t * diff, mpi_calls_types_t *current,
        mpi_calls_types_t *last);
state_t mpi_call_diff(mpi_information_t *diff, mpi_information_t *end, mpi_information_t *init);


/* This function marks processes with their timeouts (steps ) */
void configure_processes_tobe_restored();
/* Returns true if the process was in a barrier and must be 
 * restored the CPU freq */
uint process_must_be_restored();
/* Marks the process as already restored */
void process_already_restored();
/* Decrements in one step the number of pending
 * steps until the process recovers its CPU freq */
void process_new_step();
/* Returns true in the last step */
uint process_last_step();
/* Returns true when the Blocking operation is done */
uint process_block_is_ready();
/* Marks the ready operation as done */
void process_block_ready();

void process_set_target_pstate(uint i);
uint process_get_target_pstate();
void process_set_curr_mpi_pstate(uint i);
uint process_get_curr_mpi_pstate();
uint process_get_num_pstates();
void process_reset_steps();


/* MPI Optimization functions */

typedef struct mpi_opt_policy
{
  state_t (*init_mpi) (polctx_t *c, mpi_call call_type, node_freqs_t *freqs, int *process_id);
  state_t (*end_mpi)	(node_freqs_t *freqs, int *process_id);
  void    (*summary)	(int verb_lvl);
  state_t (*periodic)	(uint *to_be_changed_pstate);
} mpi_opt_policy_t;

state_t mpi_opt_load(mpi_opt_policy_t * mpi_opt_func);


#endif // MPI_SUPPORT_H
