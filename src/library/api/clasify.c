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


#include <stdio.h>
#include <stdlib.h>
//#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/hardware/defines.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <common/hardware/topology.h>
#include <library/common/verbose_lib.h>
#include <library/common/externs.h>
#include <library/dynais/dynais.h>
#include <library/api/clasify.h>
#include <library/states/states_comm.h>
#include <library/loader/module_cuda.h>
#include <library/api/mpi_support.h>
#include <library/policies/common/mpi_stats_support.h>


#define WARN_CLASSIFY  1
#define INFO_CLASSIFY  2
#define INFO2_CLASSIFY 3

#define flops_per_sig(s) (s->FLOPS[0] + s->FLOPS[1] + s->FLOPS[2] + s->FLOPS[3] +\
  s->FLOPS[4] + s->FLOPS[5] + s->FLOPS[6] + s->FLOPS[7])

extern ulong perf_accuracy_min_time;
extern uint signature_reported;

static float CPI_CPU_BOUND =       CPI_CPU_BOUND_DEF;
static float GBS_CPU_BOUND =       GBS_CPU_BOUND_DEF;
static float CPI_BUSY_WAITING =    CPI_BUSY_WAITING_DEF;
static float GBS_BUSY_WAITING =    GBS_BUSY_WAITING_DEF;
static float GFLOPS_BUSY_WAITING = GFLOPS_BUSY_WAITING_DEF;
static float IO_TH               = IO_TH_DEF;
static float CPI_MEM_BOUND       = CPI_MEM_BOUND_DEF;
static float GBS_MEM_BOUND       = GBS_MEM_BOUND_DEF;


void get_classify_limits(ear_classify_t *limits)
{
  limits->cpi_cpu_bound     = CPI_CPU_BOUND;
  limits->gbs_cpu_bound     = GBS_CPU_BOUND;
  limits->cpi_busy_waiting  = CPI_BUSY_WAITING;
  limits->gbs_busy_waiting  = GBS_BUSY_WAITING;
  limits->gflops_busy_waiting = GFLOPS_BUSY_WAITING;
  limits->io_th               = IO_TH;
  limits->mpi_th              = MAX_MPI_CALLS_SECOND;
  limits->cpi_mem_bound       = CPI_MEM_BOUND;
  limits->gbs_mem_bound       = GBS_MEM_BOUND;
}


char * phase_to_str(uint phase)
{
  switch (phase){
    case APP_COMP_BOUND   : return "COMP_BOUND";
    case APP_MPI_BOUND    :  return "MPI_BOUND";
    case APP_IO_BOUND     :  return "IO_BOUND";
    case APP_BUSY_WAITING : return "CPU_BUSY_WAITING";
    case APP_CPU_GPU      : return "CPU_GPU_COMP";
    case APP_COMP_CPU    : return "CPU_BOUND";
    case APP_COMP_MEM    : return "MEM_BOUND";
    case APP_COMP_MIX    : return "MIX_COMP";
  }
  return "NOT_PHASE";
}


state_t classify_init(topology_t *tp_in)
{
  switch (tp_in->vendor){
    case VENDOR_INTEL:
      /* Before Sylake */
      if (tp_in->model <= MODEL_BROADWELL_X){
        return EAR_SUCCESS;
      }
      /* Skylake */
      if (tp_in->model < MODEL_ICELAKE_X){
        return EAR_SUCCESS;
      }
      /* Icelake */
      if (tp_in->model >= MODEL_ICELAKE_X){
        return EAR_SUCCESS;
      }
      break;
    case VENDOR_AMD:
      /* Zen2 = Rome */
      if (tp_in->family < FAMILY_ZEN3){
        return EAR_SUCCESS;
      }
      /* Zen3 = Milan */
      if (tp_in->family >= FAMILY_ZEN3){
        return EAR_SUCCESS;
      }
      break;
    default:break;
  }
  return EAR_SUCCESS;
}


state_t classify(uint iobound,uint mpibound, uint *app_state)
{
	*app_state = APP_COMP_BOUND;
	if (iobound && !mpibound){
		*app_state = APP_IO_BOUND;
	} else if (mpibound) {
		*app_state = APP_MPI_BOUND;
	}
	return EAR_SUCCESS;
}


state_t is_io_bound(signature_t *sig, uint  num_cpus, uint *iobound)
{
    float rwsec = sig->IO_MBS;
    *iobound = 0;
    if (rwsec > IO_TH) *iobound = 1;
    return EAR_SUCCESS;

}


state_t is_network_bound(signature_t *sig, uint  num_cpus, uint *mpibound)
{
	*mpibound = 0;
	if (is_mpi_intensive()){
		*mpibound = 1;
	}
	return EAR_SUCCESS;
}


static uint first_warning_mpi_th = 1;
state_t must_switch_to_time_guide(ulong last_sig_elapsed, uint *ear_guided)
{ 
    /* Changed to force the EARL to use dynais as much as possible */
    if (!is_mpi_enabled()){
      *ear_guided = TIME_GUIDED;
      return EAR_SUCCESS;
    }
    if (is_low_mpi_activity()){
      //if (!is_cuda_enabled()) *ear_guided = TIME_GUIDED;
      if (first_warning_mpi_th) {
        verbose_master(WARN_CLASSIFY, "Warning, going to time guided because low number of MPI calls per second current %f ", avg_mpi_calls_per_second());
        first_warning_mpi_th = 0;
      }
      *ear_guided = TIME_GUIDED;
    }
    if (signature_reported && (last_sig_elapsed > ((5*perf_accuracy_min_time)/1000000))){
      verbose_master(WARN_CLASSIFY, "Warning, going to time guided because elapsed time since last signature is %lu ", last_sig_elapsed);
      *ear_guided = TIME_GUIDED;
    }
    return EAR_SUCCESS;
}


extern uint validate_cpu_busy;
state_t is_process_busy_waiting(ssig_t *sig, uint *busy)
{
  if (busy == NULL) return EAR_ERROR;
  
  *busy = 0;
  if (!validate_cpu_busy) return EAR_SUCCESS;

	uint c1 = (sig->CPI    < CPI_BUSY_WAITING);
	uint c2 = (sig->GBS    < GBS_BUSY_WAITING);
	uint c3 = (sig->Gflops < GFLOPS_BUSY_WAITING);

	*busy = c1 && c2 && c3;

  /* This check is new to detect when the code is doing some FOP 
  * operation. Assumin Busy Waiting is not doint */
  #ifdef __ARCH_ARM
  if (*busy){
    if (flops_per_sig(sig)) *busy = 0;
  }
  #endif
  return EAR_SUCCESS;
}


state_t is_cpu_busy_waiting(signature_t *sig, uint num_cpus, uint *busy)
{
    
    if (!sig || !busy) {
        return_msg(EAR_ERROR, Generr.input_null);
    }



	*busy = USEFUL_COMP;

  if (!validate_cpu_busy) return EAR_SUCCESS;

	uint c1 = (sig->CPI    < CPI_BUSY_WAITING);
	uint c2 = ((sig->GBS/num_cpus)    < GBS_BUSY_WAITING);
	uint c3 = ((sig->Gflops/num_cpus) < GFLOPS_BUSY_WAITING);


    if (c1 && c2 && c3) {
        *busy = BUSY_WAITING;
         /* This check is new to detect when the code is doing some FOP 
          * operation. Assumin Busy Waiting is not doint */
        if (*busy){
        #ifdef __ARCH_ARM
          if (flops_per_sig(sig)) *busy = 0;
         #endif
        }
    }

    if (VERB_ON(INFO_CLASSIFY)) {

        if (c1 || c2 || c3) {
            char hdr_msg[] = "--- Busy waiting values (value/thresh) ---";

            int ret;
            char cpi_msg[32];
            ret = snprintf(cpi_msg, sizeof cpi_msg, "CPI class=%u (%.3lf/%.3lf)", c1, sig->CPI, CPI_BUSY_WAITING);

            char gbs_msg[32];
            ret = snprintf(gbs_msg, sizeof gbs_msg, "Mem. bwidth class=%u (%.3lf/%.3lf)", c2, sig->GBS, GBS_BUSY_WAITING);

            char gflops_msg[32];
            ret = snprintf(gflops_msg, sizeof gflops_msg, "GFLOP/s class=%u %.3lf/%.3lf)", c3, sig->Gflops, GFLOPS_BUSY_WAITING);

            char busy_waiting_msg[64];
            ret = snprintf(busy_waiting_msg, sizeof busy_waiting_msg,
                           "App node workload declared as busy waiting: %u", *busy);

            int max_strlen = strlen(busy_waiting_msg);
            /*
               int max_strlen = ear_max(strlen(hdr_msg), strlen(cpi_msg));
               max_strlen = ear_max(max_strlen, strlen(gbs_msg));
               max_strlen = ear_max(max_strlen, strlen(gflops_msg));
               */

            verbose_master(INFO_CLASSIFY, "%*s%s\n%*s%s\n%*s%s\n%*s%s\n%s\n",
                    (int) (max_strlen - strlen(hdr_msg)) / 2,    " ",  hdr_msg,
                    (int) (max_strlen - strlen(cpi_msg)) / 2,    " ",  cpi_msg,
                    (int) (max_strlen - strlen(gbs_msg)) / 2,    " ",  gbs_msg,
                    (int) (max_strlen - strlen(gflops_msg)) / 2, " ",  gflops_msg,
                    busy_waiting_msg);
        }

    }

	return EAR_SUCCESS;
}


state_t low_mem_activity(signature_t *sig, uint num_cpus, uint *lowm)
{
  if (lowm)
  {
    float per_proc_gbs = sig->GBS / num_cpus;

    *lowm = (per_proc_gbs < GBS_BUSY_WAITING);

    debug("GB/s normalized: %lf Thresh %lf", per_proc_gbs, GBS_BUSY_WAITING);

    return EAR_SUCCESS;
  } else
  {
    return EAR_ERROR;
  }
}


state_t is_cpu_bound(signature_t *sig, uint num_cpus, uint *cbound)
{
    *cbound = 0;
    if ((sig->CPI < CPI_CPU_BOUND) && (sig->GBS < GBS_CPU_BOUND)) *cbound = 1;
    //verbose_master(INFO_CLASSIFY, "CPI %lf thresh %lf GB/s %lf thresh %lf", sig->CPI, CPI_CPU_BOUND, sig->GBS, GBS_CPU_BOUND);
    return EAR_SUCCESS;
}


state_t is_mem_bound(signature_t *sig,uint num_cpus, uint *mbound)
{
    if (mbound)
    {
      *mbound = 0;
      if ((sig->CPI > CPI_MEM_BOUND) && (sig->GBS > GBS_MEM_BOUND))
      {
        *mbound = 1;
      }

      debug("CPI %lf thresh %lf GB/s %lf thresh %lf", sig->CPI, CPI_MEM_BOUND, sig->GBS, GBS_MEM_BOUND);

      return EAR_SUCCESS;
    }
    return EAR_ERROR;
}


state_t gpu_activity_state(signature_t *sig, gpu_state_t *gpu_state)
{
#if USE_GPUS
    int total = 0;
    *gpu_state = _GPU_NoGPU;
    if (sig->gpu_sig.num_gpus == 0) return EAR_SUCCESS;
    if (!is_cuda_enabled()) {
        /* One of the following cases:
         * - Node does not have GPUs
         * - Node has GPUs but Job does not use them
         */
        *gpu_state = _GPU_NoGPU;
        return EAR_SUCCESS;
    }
    *gpu_state = _GPU_Idle;
    // for sure the Job has been assigned to use GPUs, but are they being used?
	  for (int i = 0; i < sig->gpu_sig.num_gpus; i++) {
        if (sig->gpu_sig.gpu_data[i].GPU_util > 0) {
            total += sig->gpu_sig.gpu_data[i].GPU_util;
            *gpu_state = _GPU_Comp;
        }
    }
    if (total > 0){
      uint avg_gpu_util = total / sig->gpu_sig.num_gpus;
      if (avg_gpu_util > 0) *gpu_state = _GPU_Bound;
    }
#else
	  *gpu_state = _GPU_NoGPU;
#endif // USE_GPUS
	return EAR_SUCCESS;
}
