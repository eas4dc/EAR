/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#ifndef EAR_EAR_METRICS_H
#define EAR_EAR_METRICS_H

#include <common/config.h>
#include <common/hardware/topology.h>
#include <metrics/common/apis.h>
#include <metrics/io/io.h>
#include <library/common/library_shared_data.h>
#include <metrics/proc/stat.h>
#include <library/api/clasify.h>
#include <library/metrics/dcgmi_lib.h>
#if DLB_SUPPORT
#include <library/metrics/dlb_talp_lib.h>
#endif


#define MGT_CPUFREQ 1
#define MGT_IMCFREQ 2
#define MET_CPUFREQ 3
#define MET_IMCFREQ 4
#define MET_BWIDTH  5
#define MET_FLOPS   6
#define MET_CACHE   7
#define MET_CPI     8
#define MGT_GPU     9
#define MET_GPU     10
#define MET_TEMP    11


/** For each phase */
typedef struct phase_info {
  ullong elapsed;
} phase_info_t;


typedef struct sig_ext
{
    io_data_t          iod;
    mpi_information_t *mpi_stats;
    mpi_calls_types_t *mpi_types;
    float              max_mpi, min_mpi;
    float              elapsed;
    float              telapsed;
    float              saving;
    float              psaving;
    float              tpenalty;
    phase_info_t       earl_phase[EARL_MAX_PHASES];
    dcgmi_sig_t        dcgmis;
#if DLB_SUPPORT
		earl_talp_t				 earl_talp_data;
#endif
		proc_stat_t        proc_ps;
		ulong              sel_mem_freq_khz;
} sig_ext_t;

/** New metrics **/
const metrics_t *metrics_get(uint api);

#if USE_GPUS
const metrics_gpus_t *metrics_gpus_get(uint api);
#endif

/** Returns the current time in usecs */
long long metrics_time();

/** Returns the difference between times in usecs */
long long metrics_usecs_diff(long long end, long long init);

/** Initializes local metrics as well as daemon's metrics */
int metrics_load(topology_t *topo);

/** Stops metrics collection and computes the accumulated data*/
void metrics_dispose(signature_t *metrics, ulong procs);

/** Restarts the current metrics and recomputes the signature */
void metrics_compute_signature_begin();

/** */
state_t metrics_compute_signature_finish(signature_t *metrics, uint iterations, ulong min_time_us, ulong procs, llong *passed_time);

/** Estimates whether the current time running the loops is enough to compute the signature */
int time_ready_signature(ulong min_time_us);

/** Copute the number of vector instructions since signature reports FP ops, metrics is valid signature*/
unsigned long long metrics_vec_inst(signature_t *metrics);

/** Computes the job signature including data from other processes. */
void metrics_job_signature(const signature_t *master, signature_t *dst);

/** Computes the node signature at app end */
void metrics_app_node_signature(signature_t *master,signature_t *ns);

/** Computes the per-process and per-job metrics using power models for node sharing. */
void compute_per_process_and_job_metrics(signature_t *sig);

/* Computes metrics per-iteration, very lightweight */
state_t metrics_new_iteration(signature_t *sig);

/* Resets metrics for new processes , after fork*/
void metrics_lib_reset();

#if MPI_OPTIMIZED
void metrics_start_cpupower();

void metrics_read_cpupower();
#endif


#if DCGMI
uint metrics_dcgmi_enabled();
#endif

/* Returns the number of iterations per second in the last computed signature */
float metrics_iter_per_second();


uint metrics_CPU_phase_ok();

#endif //EAR_EAR_METRICS_H
