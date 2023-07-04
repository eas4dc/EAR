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

#define SHOW_DEBUGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <common/config.h>
#include <common/states.h>
#include <common/output/debug.h>

#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
//#include <library/metrics/gpu.h>
#include <management/gpu/gpu.h>
#include <metrics/gpu/gpu.h>
#include <library/metrics/metrics.h>
#include <library/api/clasify.h>
#include <library/policies/policy_api.h>
#include <library/policies/policy_state.h>
#include <library/policies/common/cpu_support.h>

#ifdef EARL_RESEARCH
extern unsigned long ext_def_freq;
#define DEF_FREQ(f) (!ext_def_freq?f:ext_def_freq)
#else
#define DEF_FREQ(f) f
#endif

#if 0
#define debug(...) \
{ \
        dprintf(2, __VA_ARGS__); \
        dprintf(2, "\n"); \
}
#endif

#define LOW_UTIL_GPU_TH 85
#define MED_UTIL_GPU_TH 95
#define HIGH_UTIL_GPU_TH 75
#define LOW_GPU_FREQ_RED 0.5
#define MED_GPU_FREQ_RED 0.25
#define HIGH_GPU_FREQ_RED 0.1

#define FIRST_SELECTION 0
#define LINEAR_SEARCH   1
#define REFERENCE_SIG   2
#define GPU_ENABLE_OPT "EAR_GPU_ENABLE_OPT"
#define GPU_PSTATE_JUMP 4

static uint *GPU_policy_state;
static ulong  *last_gpu_util;
static uint   *last_gpu_pstate;
static double *last_gpu_power;

static const ulong **gpuf_list;
static const uint *gpuf_list_items;
static ulong *gfreqs;

extern gpu_state_t last_gpu_state;
static uint gpu_optimize = 0;

state_t policy_init(polctx_t *c)
{
    int i,j;
    ulong g_freq = 0;
    char *gpu_freq=getenv(FLAG_GPU_DEF_FREQ);
    if ((gpu_freq!=NULL) && (c->app->user_type==AUTHORIZED)){
        g_freq=atol(gpu_freq);
    }
    //gpu_lib_freq_list(&gpuf_list, &gpuf_list_items);
    gpuf_list       = (const ulong **) metrics_gpus_get(MGT_GPU)->avail_list;
    gpuf_list_items = (const uint *)   metrics_gpus_get(MGT_GPU)->avail_count;

    for (i=0;i<c->num_gpus;i++){
        verbose_master(3,"Freqs in GPU[%d]=%u",i,gpuf_list_items[i]);
        for (j=0;j<gpuf_list_items[i];j++){
            verbose_master(3,"GPU[%d][%d]=%.2f",i,j,(float)gpuf_list[i][j]/1000000.0);
        }
    }

    last_gpu_util = calloc(c->num_gpus, sizeof(ulong));
    last_gpu_pstate = calloc(c->num_gpus, sizeof(uint));
    GPU_policy_state = calloc(c->num_gpus, sizeof(uint));
    last_gpu_power = calloc(c->num_gpus, sizeof(double));

    mgt_gpu_data_alloc(&gfreqs);

    for (i=0;i<c->num_gpus;i++){
        GPU_policy_state[i] = FIRST_SELECTION;
        if (g_freq) gfreqs[i] = g_freq;
        else 				gfreqs[i] = gpuf_list[i][0];
        verbose_master(2,"Setting GPUgreq[%d] = %.2f",i,(float)gfreqs[i]/1000000.0);
    }

    mgt_gpu_freq_limit_set(no_ctx, gfreqs);
    char *cgpu_optimize = getenv(GPU_ENABLE_OPT);
    if (cgpu_optimize != NULL) gpu_optimize = atoi(cgpu_optimize);
    verbose_master(2,"GPU optimization during GPU comp %s", (gpu_optimize? "Enabled":"Disabled"));


    return EAR_SUCCESS;
}

static uint GPU_ok(uint i, signature_t *my_sig)
{
  uint gpu_next_eval = (my_sig->gpu_sig.gpu_data[i].GPU_util >= last_gpu_util[i]);
  debug("GPU_next_eval based on util %u (util last %lu util current %lu)", gpu_next_eval, last_gpu_util[i], my_sig->gpu_sig.gpu_data[i].GPU_util);
  if (gpu_next_eval) return gpu_next_eval;

  gpu_next_eval = (my_sig->gpu_sig.gpu_data[i].GPU_freq < gpuf_list[i][last_gpu_pstate[i]] - (gpuf_list[i][last_gpu_pstate[i]] * 0.03));
  debug("GPU_next_eval based on freq %u (avg %lu selected %lu)", gpu_next_eval, my_sig->gpu_sig.gpu_data[i].GPU_freq, gpuf_list[i][last_gpu_pstate[i]]); 
  if (gpu_next_eval) return gpu_next_eval;

  /* We compute the variation of utilization vs power variation */
  float util_var  = last_gpu_util[i] - my_sig->gpu_sig.gpu_data[i].GPU_util;

  /* We accept upo to 5% of difference in utilization (in total)*/
  gpu_next_eval =  (util_var <  5);
  if (gpu_next_eval) return gpu_next_eval;

  float power_var = 100.0*(last_gpu_power[i] - my_sig->gpu_sig.gpu_data[i].GPU_power)/last_gpu_power[i];
  debug("GPU Power variation %.2f . GPU util variation %.2f", power_var, util_var);
  /* We accept up to 10% in power (int total)*/
  gpu_next_eval = (power_var < 10);

  //gpu_next_eval = (power_var > util_var);

  debug("GPU_next_eval based on power %u", gpu_next_eval);

  return gpu_next_eval;
}
static double cpi_ref;
static double time_ref;
static double gbs_ref;
static double gpu_penalty = 0.02;
static signature_t prev_gpu_sig;

static uint CPU_ok(signature_t *my_sig)
{
  uint CPU_next_eval = policy_no_models_is_better_min_energy(my_sig, &prev_gpu_sig, time_ref,
      cpi_ref, gbs_ref, gpu_penalty);
  debug("CPU_next_eval %u", CPU_next_eval);
  return CPU_next_eval;
}

state_t policy_apply(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, int *ready)
{
    ulong *new_freq=freqs->gpu_freq;
    uint i;
    ulong util, tutil = 0;;
    *ready = EAR_POLICY_READY;
#if USE_GPUS
    verbose_master(2, "%sGPU Min_energy_to_solution...........starts.......%s", COL_GRE, COL_CLR);
#if SHOW_DEBUGS
    debug("apply: num_gpus %d",my_sig->gpu_sig.num_gpus);
    for (i = 0; i < my_sig->gpu_sig.num_gpus; i++) {
        debug("[GPU %d Power %.2lf Freq %.2f Mem_freq %.2f Util %lu Mem_util %lu] policy state %u", i,
                my_sig->gpu_sig.gpu_data[i].GPU_power,
                (float) my_sig->gpu_sig.gpu_data[i].GPU_freq / 1000.0,
                (float) my_sig->gpu_sig.gpu_data[i].GPU_mem_freq/1000.0,
                my_sig->gpu_sig.gpu_data[i].GPU_util, my_sig->gpu_sig.gpu_data[i].GPU_mem_util, GPU_policy_state[i]);
    }
#endif // SHOW_DEBUGS

    for (i=0; i < my_sig->gpu_sig.num_gpus; i++) {
        util = my_sig->gpu_sig.gpu_data[i].GPU_util;
        if (util == 0) {
            debug("GPU idle");
            GPU_policy_state[i] = FIRST_SELECTION;
            new_freq[i] = gpuf_list[i][gpuf_list_items[i]-1];
        } else {
            if (!gpu_optimize){
              debug("GPU not optimized");
              new_freq[i] = gfreqs[i];
            }else{
              if (GPU_policy_state[i] == FIRST_SELECTION){
                debug("Starting linear search for GPU %d", i);
                new_freq[i] = gpuf_list[i][0];
                last_gpu_pstate[i] = 0;
                last_gpu_util[i]   = util;
                last_gpu_power[i]  = my_sig->gpu_sig.gpu_data[i].GPU_power;
                GPU_policy_state[i] = LINEAR_SEARCH;
                /* References for penalty estimation */
                cpi_ref = my_sig->CPI;
                gbs_ref = my_sig->GBS;
                time_ref = my_sig->time;
                signature_copy(&prev_gpu_sig, my_sig);
                *ready = EAR_POLICY_TRY_AGAIN;
              }else{
                //if (GPU_ok(i,my_sig) && CPU_ok(my_sig)){
                if (CPU_ok(my_sig)){
                  debug("Reducing the GPU frequency for GPU %d", i);
                  new_freq[i] = gpuf_list[i][last_gpu_pstate[i] + GPU_PSTATE_JUMP];
                  last_gpu_pstate[i] += GPU_PSTATE_JUMP;
                  last_gpu_power[i]  = my_sig->gpu_sig.gpu_data[i].GPU_power;
                  signature_copy(&prev_gpu_sig, my_sig);
                  *ready = EAR_POLICY_TRY_AGAIN;
                }else{
                  new_freq[i] = gpuf_list[i][last_gpu_pstate[i]];
                  GPU_policy_state[i] = FIRST_SELECTION;
                  debug("GPU freq done for GPU %d", i);
                }
            }

            tutil += util; // Accumulate GPU utilization
        } /* GPU extra opt */
      } /* GPU comp */
    } /* GPU loop */

    if (tutil > 0) {
        last_gpu_state = _GPU_Comp;
    }
#endif // USE_GPUS
    return EAR_SUCCESS;
}



state_t policy_restore_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
    ulong util, tutil = 0;;
    int i;

    if (c == NULL) return EAR_ERROR;

#if USE_GPUS
    verbose_master(2, "Restoring GPU freqs: restore settings");
    for (i=0;i<my_sig->gpu_sig.num_gpus;i++) {
        util = my_sig->gpu_sig.gpu_data[i].GPU_util;
        tutil += util;
        if (util == 0){
            freqs->gpu_freq[i] = gpuf_list[i][gpuf_list_items[i]-1];
        }else{
            freqs->gpu_freq[i] = gfreqs[i];
        }
        GPU_policy_state[i] = FIRST_SELECTION;
    }
    if (tutil > 0) {
        last_gpu_state = _GPU_Comp;
    }

#endif // USE_GPUS
    return EAR_SUCCESS;
}

#if USE_GPUS
extern uint  gpu_ready;
state_t policy_busy_wait_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
  ulong *new_freq = freqs->gpu_freq;
  gpu_ready = EAR_POLICY_READY;
  if (c == NULL) {
    return EAR_ERROR;
  }
  for (uint i = 0 ; i < my_sig->gpu_sig.num_gpus ; i++){
    if (my_sig->gpu_sig.gpu_data[i].GPU_util == 0){
      new_freq[i] = gpuf_list[i][gpuf_list_items[i]-1]; 
    }else{
      new_freq[i] = gpuf_list[i][last_gpu_pstate[i]];
    }
  }
  if (!gpu_optimize) return EAR_SUCCESS;

  uint i;
  debug("GPU min_energy: CPU busy waiting phase");
  for (i=0;i<my_sig->gpu_sig.num_gpus;i++) {
    if (my_sig->gpu_sig.gpu_data[i].GPU_util){
      if (GPU_policy_state[i] == FIRST_SELECTION){
        debug("Starting linear search for GPU %d", i);
        new_freq[i] = gpuf_list[i][0];
        last_gpu_pstate[i] = 0;
        last_gpu_util[i]   = my_sig->gpu_sig.gpu_data[i].GPU_util;
        last_gpu_power[i]  = my_sig->gpu_sig.gpu_data[i].GPU_power;
        GPU_policy_state[i] = REFERENCE_SIG;
        gpu_ready = EAR_POLICY_TRY_AGAIN;
      }else if (GPU_policy_state[i] == REFERENCE_SIG){
        last_gpu_util[i]   = my_sig->gpu_sig.gpu_data[i].GPU_util;
        last_gpu_power[i]  = my_sig->gpu_sig.gpu_data[i].GPU_power;
        new_freq[i] = gpuf_list[i][last_gpu_pstate[i] + GPU_PSTATE_JUMP];
        last_gpu_pstate[i] += GPU_PSTATE_JUMP;
        gpu_ready = EAR_POLICY_TRY_AGAIN;
        GPU_policy_state[i] = LINEAR_SEARCH;
      }else if (GPU_policy_state[i] == LINEAR_SEARCH){
        //if (GPU_ok(i,my_sig) && CPU_ok(my_sig)){
        if (GPU_ok(i,my_sig)){
          debug("Reducing the GPU frequency for GPU %d", i);
          new_freq[i] = gpuf_list[i][last_gpu_pstate[i] + GPU_PSTATE_JUMP];
          last_gpu_pstate[i] += GPU_PSTATE_JUMP;
          signature_copy(&prev_gpu_sig, my_sig);
          gpu_ready = EAR_POLICY_TRY_AGAIN;
        }else{
          new_freq[i] = gpuf_list[i][last_gpu_pstate[i]];
          GPU_policy_state[i] = FIRST_SELECTION;
          debug("GPU freq done for GPU %d", i);
        }
      }
    }
  }
  return EAR_SUCCESS;
 
}

state_t policy_cpu_gpu_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
  if (c == NULL) return EAR_ERROR;
  verbose_master(2,"%sCPU-GPU  phase  GPU min_energy  %s",COL_BLU,COL_CLR);
  return policy_apply(c, my_sig, freqs, &gpu_ready);
}
#endif


