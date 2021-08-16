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
#include <library/api/mpi_support.h>
#include <library/policies/policy_ctx.h>
#include <library/common/library_shared_data.h>

extern float LB_TH;

state_t mpi_app_init(polctx_t *c);
state_t mpi_app_end(polctx_t *c);
state_t mpi_call_init(polctx_t *c,mpi_call call_type);
state_t mpi_call_end(polctx_t *c,mpi_call call_type);


state_t mpi_call_read_diff(mpi_information_t *diff,mpi_information_t *last,int i);
state_t mpi_call_new_phase(polctx_t *c);


state_t mpi_call_get_stats(mpi_information_t *dst,mpi_information_t * src,int i);
state_t mpi_call_get_types(mpi_calls_types_t *dst,shsignature_t * src,int i);

state_t clean_my_mpi_type_info(mpi_calls_types_t *t);
state_t mpi_call_types_read_diff(mpi_calls_types_t *diff,mpi_calls_types_t *last, int i);
/* Per-Node Executes the read-diff for mpi statistics on mpi calls types */
state_t read_diff_node_mpi_type_info(lib_shared_data_t *data,shsignature_t *sig,mpi_calls_types_t *node_mpi_calls,mpi_calls_types_t *last_node);

void verbose_mpi_types(int vl,mpi_calls_types_t *t);
void verbose_node_mpi_types(int vl,uint nump,mpi_calls_types_t *t);
void verbose_mpi_data(int vl,mpi_information_t *mc);

state_t copy_node_mpi_data(lib_shared_data_t *data,shsignature_t *sig,mpi_information_t *node_mpi_calls);

/* Per-Node Executes the read-diff for mpi statistics */
state_t read_diff_node_mpi_data(lib_shared_data_t *data,shsignature_t *sig,mpi_information_t *node_mpi_calls,mpi_information_t *last_node);

/*  Computes the the max and min process with perc mpi calls from percs_mpi */
state_t mpi_support_perc_mpi_max_min(int nump, double *node_mpi_calls,int *max,int *min);

/*  Computes basic statistics of percs_mpi */
state_t mpi_support_compute_mpi_lb_stats(double *percs_mpi, int num_procs, double *mean, double *sd, double *mag);

/*  Based on perc. of mpi calls for each of 'num_procs' passed,
 *  compute basic statistics (therefore stores their results) and determines whether the application is load balanced. */
state_t mpi_support_evaluate_lb(mpi_information_t *node_mpi_calls, int num_procs, double *percs_mpi, double *mean, double *sd, double *mag, uint *lb);

/*  This function sets the critical path and also stores max and min perc_mpi processes */
state_t mpi_support_select_critical_path(uint *critical_path, double *percs_mpi, int num_procs, double mean, double *median, int *max_mpi, int *min_mpi);

state_t mpi_support_verbose_perc_mpi_stats(int verb_lvl, double *percs_mpi, int num_procs, double mean, double median, double sd, double mag);

/*  This function checks whether current mpi data has changed from previous iteration */
state_t mpi_support_mpi_changed(double current_mag, double prev_mag, uint *cp, uint *prev_cp, int num_procs, double *similarity, uint *mpi_changed);

/*  Statistics */
state_t mpi_stats_evaluate_sd(int nump, double* percs_mpi, double mean, double *sd);
state_t mpi_stats_evaluate_mean(int nump, double *percs_mpi, double *mn);
state_t mpi_stats_get_only_percs_mpi(mpi_information_t *node_mpi_calls, int nump, double *percs_mpi);
state_t mpi_stats_evaluate_similarity(double *current_perc_mpi, double *last_perc_mpi, size_t size, double *similarity);
state_t mpi_stats_evaluate_sim_uint(uint *current_cp, uint *last_cp, size_t size, double *sim);
state_t mpi_stats_sort_perc_mpi(double *perc_mpi_arr, size_t nump);
state_t mpi_stats_evaluate_mean_sd(int nump, double *percs_mpi, double *mean, double *sd);
state_t mpi_stats_evaluate_mean_sd_mag(int nump, double *percs_mpi, double *mean, double *sd, double *mag);
state_t mpi_stats_evaluate_median(int nump, double *percs_mpi, double *median);
float compute_mpi_in_period(mpi_information_t *dst);


state_t is_blocking_busy_waiting(uint *block_type);

void get_last_mpi_summary(mpi_summary_t *l);
void verbose_mpi_summary(int vl,mpi_summary_t *l);
void chech_node_mpi_summary();




#endif
