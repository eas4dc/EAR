/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef APP_CLASSIFICATION
#define APP_CLASSIFICATION

#include <stdio.h>
#include <stdlib.h>
#include <common/config.h>
#include <common/hardware/topology.h>
#include <common/types/signature.h>
#include <common/types/event_type.h>
#include <common/types/classification_limits.h>



#define EARL_MAX_PHASES   9
#define EARL_BASIC_PHASES 6

#define DYNAIS_GUIDED 0
#define TIME_GUIDED	  1

#define USEFUL_COMP  0
#define BUSY_WAITING 1
// #define CPI_CPU_BOUND 0.4

//#define FLOPS_CPU_BOUND 100.0
//

#define MAX_GBS_DIFF        30

typedef enum {
    _GPU_NoGPU = 1,
    _GPU_Comp  = 2,
    _GPU_Idle  = 4,
    _GPU_Bound = 8,
} gpu_state_t;


typedef struct ear_classify {
  float cpi_cpu_bound ;
  float gbs_cpu_bound;
  float cpi_busy_waiting;
  float gbs_busy_waiting;
  float gflops_busy_waiting;
  float io_th;
  float mpi_th;
  float cpi_mem_bound;
  float gbs_mem_bound;
  float mpi_bound;
} ear_classify_t;

#define NODE    0
#define PROCESS 1

/* Initializes data based on topology */
state_t classify_init(topology_t *tp_in);

/* Get curren limits */
void get_classify_limits(ear_classify_t *limits);

/* Classify the app in APP_COMP_BOUND, APP_IO_BOUND or APP_MPI_BOUND . To review */
state_t classify(uint iobound, uint mibound, uint *app_state);

/* Checks based on CPI, GBs and GFlops, per Node but normalized */
state_t is_cpu_busy_waiting(signature_t *sig, uint num_cpus, uint *busy);

/* CPU computation (low util) or GPU Bound (high util) , or no Util */
state_t gpu_activity_state(signature_t *sig, gpu_state_t *gpu_state);
/* Checks based on CPI, GBs and GFlops, not normalize */
state_t is_process_busy_waiting(ssig_t *sig, uint *busy);
/* Checks CPI and GBs, per node */
state_t is_cpu_bound(signature_t *sig,uint num_cpus, uint *cbound);
/* Check CPI and GBS, per node */
state_t is_mem_bound(signature_t *sig,uint num_cpus, uint *mbound);
/* Based on signature IO_MBS */
state_t is_io_bound(signature_t *sig, uint num_cpus, uint *iobound);
/* Based on perc_MPI */
state_t is_network_bound(signature_t *sig, uint num_cpus, uint *mpibound);
/* Checks the per-process GBs */
state_t low_mem_activity(signature_t *sig,uint num_cpus , uint *lowm);
/* Returns the text associated with the phase */
char * phase_to_str(uint phase);


state_t must_switch_to_time_guide(ulong last_sig_elapsed, uint *ear_guided);


#endif // APP_CLASSIFICATION
