/*
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

#ifndef MPI_SUPPORT_H
#define MPI_SUPPORT_H

/** This file provides utility functions to work with MPI data. */

#include <library/api/mpi_support.h>
#include <library/policies/policy_ctx.h>
#include <library/common/library_shared_data.h>

extern float LB_TH;

/** Called on MPI_Init */
state_t mpi_app_init(polctx_t *c);
/** Called on MPI_Finalize */
state_t mpi_app_end(polctx_t *c);
/** Called at each MPI call */
state_t mpi_call_init(polctx_t *c, mpi_call call_type);
/** Called once a MPI call just ended */
state_t mpi_call_end(polctx_t *c, mpi_call call_type);

/** Per-Node Executes the read-diff for mpi statistics on mpi calls types */
state_t read_diff_node_mpi_types_info(lib_shared_data_t *data, shsignature_t *sig,
        mpi_calls_types_t *node_mpi_calls, mpi_calls_types_t *last_node);
/* Per-Node Executes the read-diff for mpi statistics */
state_t read_diff_node_mpi_info(lib_shared_data_t *data, shsignature_t *sig,
        mpi_information_t *node_mpi_calls, mpi_information_t *last_node);

state_t is_blocking_busy_waiting(uint *block_type);

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
state_t mpi_support_evaluate_lb(mpi_information_t *node_mpi_calls, int num_procs,
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

/** Returns the number of total_mpi_calls normalized by a period, e.g., 10 seconds. */
float compute_mpi_in_period(mpi_information_t *mc);
#endif // MPI_SUPPORT_H
