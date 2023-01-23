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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

#ifndef EAR_EAR_METRICS_H
#define EAR_EAR_METRICS_H

#include <common/hardware/topology.h>
#include <common/types/application.h>
#include <library/common/library_shared_data.h>
#include <metrics/io/io.h>
#include <library/api/clasify.h>

/* For each phase */
typedef struct phase_info{
  ullong elapsed;
}phase_info_t;

typedef struct sig_ext {
    io_data_t iod;
    mpi_information_t *mpi_stats;
    mpi_calls_types_t *mpi_types;
	float             max_mpi, min_mpi;
	float             elapsed;
	float             telapsed;
	float             saving;
	float             psaving;
	float             tpenalty;
  phase_info_t      earl_phase[EARL_MAX_PHASES];
} sig_ext_t;

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
int metrics_init(topology_t *topo);

/** Stops metrics collection and computes the accumulated data*/
void metrics_dispose(signature_t *metrics, ulong procs);

/** Restarts the current metrics and recomputes the signature */
void metrics_compute_signature_begin();

/** */
state_t metrics_compute_signature_finish(signature_t *metrics, uint iterations, ulong min_time_us, ulong procs);

/** Estimates whether the current time running the loops is enough to compute the signature */
int time_ready_signature(ulong min_time_us);

/** Copute the number of vector instructions since signature reports FP ops, metrics is valid signature*/
unsigned long long metrics_vec_inst(signature_t *metrics);

/* Computes the job signature including data from other processes. */
void metrics_job_signature(const signature_t *master, signature_t *dst);

/* Computes the node signature at app end */
void metrics_app_node_signature(signature_t *master,signature_t *ns);

/* CoOmputes the per-process and per-job metrics using power models for node sharing */
void compute_per_process_and_job_metrics(signature_t *sig);

/* Computes metrics per-iteration, very lightweight */
state_t metrics_new_iteration(signature_t *sig);


#if MPI_OPTIMIZED
void metrics_start_cpupower();
void metrics_read_cpupower();
#endif

#endif //EAR_EAR_METRICS_H
