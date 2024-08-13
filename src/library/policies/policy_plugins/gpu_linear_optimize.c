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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <common/config.h>
#include <common/states.h>
#include <common/output/debug.h>

#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <management/gpu/gpu.h>
#include <metrics/gpu/gpu.h>
#include <metrics/flops/flops.h>
#include <library/metrics/metrics.h>
#include <library/api/clasify.h>
#include <library/policies/policy_api.h>
#include <library/policies/policy_state.h>

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

#define GPU_PSTATE_JUMP 2
#define FIRST_SELECTION 0
#define LINEAR_SEARCH   1
#define PARTIAL_READY   2

#define FP64_SET  0
#define FP32_SET  1
#define FP16_SET  1
#define FP64_EV   1
#define FP32_EV   0
#define FP16_EV   1
#define SM_SET    2
#define SM_EV     1

static const ulong **gpuf_list;
static const uint *gpuf_list_items;
static ulong *gfreqs;

extern gpu_state_t last_gpu_state;
extern uint gpu_ready;
extern uint gpu_optimize;
static uint *GPU_policy_state;
static uint   *last_GPU_pstate;
static signature_t prev_gpu_sig;
static float *curr_value, *last_value;
static uint set_instances_cnt = 0;
static uint dcgmi_ref_set;

/* Configuration */
typedef enum {
  UTIL_W      = 0,
  FLOPS_W     = 1,
  GFLOPS_W    = 2,
	SM_OCC      = 3,
}gpu_opt_metric_t;

typedef enum{
  max_value = 0,
}gpu_opt_type_t;
static gpu_opt_metric_t metric_to_optimize = UTIL_W;
static gpu_opt_type_t   metric_strategy    = max_value;
static char optimize_criteria[1024];

#define FLAG_GPU_METRIC_TO_OPTIMIZE   "EAR_GPU_METRIC_TO_OPTIMIZE"
#define FLAG_GPU_STRATEGY_TO_OPTIMIZE "EAR_GPU_STRATEGY_TO_OPTIMIZE"

#define metric_is(x)    (x == UTIL_W?"UTIL_W":(x == FLOPS_W?"FLOPS_W":(x == GFLOPS_W?"GPU_GFLOPS_W":(x == SM_OCC?"SM_OCC":"UNDEF"))))
#define strategy_is(x)  (x == max_value?"MAX":"UNDEF")

static void update_optimize_options(char *metric,char * strategy)
{
  if (metric != NULL){
    if (strcmp(metric, "UTIL_W") == 0)       	metric_to_optimize = UTIL_W;
    if (strcmp(metric, "GFLOPS_W") == 0)      metric_to_optimize = FLOPS_W;
    if (strcmp(metric, "GPU_GFLOPS_W") == 0)  metric_to_optimize = GFLOPS_W;
    if (strcmp(metric, "SM_OCC") == 0)      	metric_to_optimize = SM_OCC;
  }
  if (strategy != NULL){
    if (strcmp(strategy, "MAX") == 0)       metric_strategy = max_value;
  }
  sprintf(optimize_criteria,"GPU Metric=%s GPU Strategy=%s", metric_is(metric_to_optimize), strategy_is(metric_strategy));
}



state_t policy_init(polctx_t *c)
{
    int i,j;
    ulong g_freq = 0;
    char *gpu_freq=ear_getenv(FLAG_GPU_DEF_FREQ);
    if ((gpu_freq!=NULL) && (c->app->user_type==AUTHORIZED)){
        g_freq=atol(gpu_freq);
    }

    char *cmetric_to_optimize   = ear_getenv(FLAG_GPU_METRIC_TO_OPTIMIZE);
    char *cstrategy_to_optimize = ear_getenv(FLAG_GPU_STRATEGY_TO_OPTIMIZE);

    /* This function checks the metric and strategy (max, min) */
    update_optimize_options(cmetric_to_optimize, cstrategy_to_optimize);

    //gpu_lib_freq_list(&gpuf_list, &gpuf_list_items);
    gpuf_list       = (const ulong **) metrics_gpus_get(MGT_GPU)->avail_list;
    gpuf_list_items = (const uint *)   metrics_gpus_get(MGT_GPU)->avail_count;

    for (i=0;i<c->num_gpus;i++){
        verbose_master(3,"Freqs in GPU[%d]=%u",i,gpuf_list_items[i]);
        for (j=0;j<gpuf_list_items[i];j++){
            verbose_master(3,"GPU[%d][%d]=%.2f",i,j,(float)gpuf_list[i][j]/1000000.0);
        }
    }

    mgt_gpu_data_alloc(&gfreqs);

    last_GPU_pstate   = (uint *)calloc(c->num_gpus, sizeof(uint));
    GPU_policy_state  = (uint *)calloc(c->num_gpus, sizeof(uint));
    last_value        = (float *)calloc(c->num_gpus, sizeof(float));
    curr_value        = (float *)calloc(c->num_gpus, sizeof(float));

    for (i=0;i<c->num_gpus;i++){
        if (g_freq) gfreqs[i] = g_freq;
        else 				gfreqs[i] = gpuf_list[i][0];
        GPU_policy_state[i] = FIRST_SELECTION;
        verbose_master(2,"Setting GPUgreq[%d] = %.2f",i,(float)gfreqs[i]/1000000.0);
    }

    /* This must be updated when adding more metrics */
    dcgmi_ref_set = ear_max(ear_max(FP64_SET, FP32_SET), FP16_SET);

    mgt_gpu_freq_limit_set(no_ctx, gfreqs);
    verbose_master(2,"GPU optimization during GPU comp %s", (gpu_optimize? "Enabled":"Disabled"));


    return EAR_SUCCESS;
}

static void GPU_get_metric(uint g, signature_t *my_sig, float *curr)
{
#if DCGMI
  sig_ext_t *sigex;
  dcgmi_sig_t *dcgmis;
#endif
  *curr = 0;
  switch(metric_to_optimize){
    case UTIL_W   : *curr = (float)(my_sig->gpu_sig.gpu_data[g].GPU_util*gpuf_list[g][last_GPU_pstate[g]])/(float)my_sig->DC_power;break;
		case SM_OCC:
    {
			#if DCGMI
			if (my_sig == NULL) return;
			sigex = (sig_ext_t *)my_sig->sig_ext;
			if (sigex == NULL)  return;
      dcgmis = (dcgmi_sig_t *)&sigex->dcgmis;
      if (dcgmis == NULL) return;
			gpuprof_t *dcgmis_data_sm = dcgmis->event_metrics_sets[SM_SET];
			float sm_occ = (float)(dcgmis_data_sm[g].values[SM_EV]);
			*curr = sm_occ * gpuf_list[g][last_GPU_pstate[g]]/(float)my_sig->DC_power;
			#endif
			break;
    }
    case GFLOPS_W:
    case FLOPS_W  : {
      #if DCGMI
      if (my_sig == NULL) return;
      sigex = (sig_ext_t *)my_sig->sig_ext;
      if (sigex == NULL)  return;
      dcgmis = (dcgmi_sig_t *)&sigex->dcgmis;
      if (dcgmis == NULL) return;
      float cpu_gflops = 0;
      float f64,f32,f16;
      ullong cpu_flops = 0;
      gpuprof_t *dcgmis_data_64 = dcgmis->event_metrics_sets[FP64_SET];
      gpuprof_t *dcgmis_data_32 = dcgmis->event_metrics_sets[FP32_SET];
      gpuprof_t *dcgmis_data_16 = dcgmis->event_metrics_sets[FP16_SET];
      verbose_master(2, "GPU[%u] computing CPU gflops ", g);
			if (metric_to_optimize == FLOPS_W){
      //	for (uint fe = 0; fe < FLOPS_EVENTS; fe++) cpu_flops += my_sig->FLOPS[fe];
      	cpu_gflops = (float)my_sig->Gflops;
			}
      verbose_master(2, "GPU[%u] computing GPU gflops ", g);
      //cpu_gflops = (float)cpu_flops/(float)1000000.0;
      f64        = (float)(dcgmis_data_64[g].values[FP64_EV]*64);
      f32        = (float)(dcgmis_data_32[g].values[FP32_EV]*32);
      f16        = (float)(dcgmis_data_16[g].values[FP16_EV]*16);
      verbose_master(2, " GPU[%u] data for optimization: %f CPU-Flops GPU (%f 64 %f 32 %f 16) elapsed %f", g, cpu_gflops, f64, f32, f16, sigex->elapsed);
      *curr = (cpu_gflops + (f64 + f32 +f16)*gpuf_list[g][last_GPU_pstate[g]]/(float)1000000.0)/(float)my_sig->DC_power ;
      verbose_master(2, " GPU[%u] data for optimization: %f",g, *curr);
      break;
      #else
      *curr = (float)0;
      #endif
    }
    default    :  *curr = (float)0;
  }
}

static uint GPU_metric_ready(signature_t *sig, uint g)
{
  switch(metric_to_optimize){
  case UTIL_W   :return 1;
	case GFLOPS_W :
  case FLOPS_W  :
  case SM_OCC   :
  {
  #if DCGMI
    sig_ext_t *sigex;
    dcgmi_sig_t *dcgmis;
    if (sig == NULL) return 0;
    sigex = (sig_ext_t *)sig->sig_ext;
    if (sigex == NULL)  return 0;
    dcgmis = (dcgmi_sig_t *)&sigex->dcgmis;
    if (dcgmis == NULL) return 0;
    if ( dcgmis->set_instances_cnt[dcgmi_ref_set] > set_instances_cnt)  return 1;
    else                    return 0;
  #endif
  }
  }
  return 0;
}

static void GPU_metric_ready_update(signature_t *my_sig)
{
  switch(metric_to_optimize){
  case UTIL_W   :return ;
  case FLOPS_W  :
  case GFLOPS_W :
  case SM_OCC   :
  {
  #if DCGMI
    sig_ext_t *sigex;
    dcgmi_sig_t *dcgmis;
    if (my_sig == NULL) return ;
    sigex = (sig_ext_t *)my_sig->sig_ext;
    if (sigex == NULL)  return ;
    dcgmis = (dcgmi_sig_t *)&sigex->dcgmis;
    if (dcgmis == NULL) return ;
    verbose_master(2,"Updating DCGMI number of instances to %d", dcgmis->set_instances_cnt[dcgmi_ref_set]);
    set_instances_cnt = dcgmis->set_instances_cnt[dcgmi_ref_set];
  #endif
  }
  }
  return;
}

static uint GPU_ok(uint g, signature_t *my_sig, float curr, float last)
{
  verbose_master(2, "GPU ok validation curr %f last %f", curr, last);
  switch (metric_strategy){
  case max_value : return (curr > last);
  default        : return 0;
  }
  return 0;
}

state_t policy_apply(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, int *ready)
{
    ulong *new_freq=freqs->gpu_freq;
    uint i;
    ulong util, tutil = 0;;
    *ready = EAR_POLICY_READY;
#if USE_GPUS
    verbose_master(2,"%sGPU Optimize start ....%s %s",COL_BLU, optimize_criteria, COL_CLR);
#if SHOW_DEBUGS
    debug("apply: num_gpus %d",my_sig->gpu_sig.num_gpus);
    for (i = 0; i < my_sig->gpu_sig.num_gpus; i++) {
        debug("[GPU %d Power %.2lf Freq %.2f Mem_freq %.2f Util %lu Mem_util %lu]", i,
                my_sig->gpu_sig.gpu_data[i].GPU_power,
                (float) my_sig->gpu_sig.gpu_data[i].GPU_freq / 1000.0,
                (float) my_sig->gpu_sig.gpu_data[i].GPU_mem_freq/1000.0,
                my_sig->gpu_sig.gpu_data[i].GPU_util, my_sig->gpu_sig.gpu_data[i].GPU_mem_util);
    }
#endif // SHOW_DEBUGS


    /* when using DCGMI, we must wait until the data is ready */
    uint metric_ready = 0;
    for (i=0; i < my_sig->gpu_sig.num_gpus; i++) {
        util = my_sig->gpu_sig.gpu_data[i].GPU_util;
        if (util == 0) {
            new_freq[i] = gpuf_list[i][gpuf_list_items[i]-1];
        } else {
            if (!gpu_optimize){
              debug("GPU not optimized");
              new_freq[i] = gfreqs[i];
            }else{
               if (GPU_metric_ready(my_sig,i)){
                verbose_master(2, "GPU[%d] optimize metric ready instances %d", i, set_instances_cnt);
                metric_ready = 1;
                /* GPU data ready */
                if (GPU_policy_state[i] == FIRST_SELECTION){
                  debug("Starting linear search for GPU %d", i);
                  new_freq[i] = gpuf_list[i][GPU_PSTATE_JUMP];
                  last_GPU_pstate[i] = 0;
                  GPU_get_metric(i, my_sig, &last_value[i]);
                  /* Save current metrics */
                  GPU_policy_state[i] = LINEAR_SEARCH;
                  signature_copy(&prev_gpu_sig, my_sig);
                  *ready = EAR_POLICY_TRY_AGAIN;
                }else if (GPU_policy_state[i] == LINEAR_SEARCH){
                  GPU_get_metric(i, my_sig, &curr_value[i]);
                  if (GPU_ok(i,my_sig, curr_value[i], last_value[i])){
                    debug("Reducing the GPU frequency for GPU %d", i);
                    last_GPU_pstate[i] += GPU_PSTATE_JUMP;
                    new_freq[i] = gpuf_list[i][last_GPU_pstate[i] + GPU_PSTATE_JUMP];
                      /* Save any metrics here */
                    last_value[i] = curr_value[i];
                    signature_copy(&prev_gpu_sig, my_sig);
                    *ready = EAR_POLICY_TRY_AGAIN;
                  }else{
                    new_freq[i] = gpuf_list[i][last_GPU_pstate[i]];
                    GPU_policy_state[i] = PARTIAL_READY;
                    debug("GPU freq done for GPU %d", i);
                  }
  
              }else if (GPU_policy_state[i] == PARTIAL_READY){
                new_freq[i] = gpuf_list[i][last_GPU_pstate[i]];
              }
            }else{
              /* GPU data not ready yet */
              new_freq[i] = gpuf_list[i][last_GPU_pstate[i]];
              *ready = EAR_POLICY_TRY_AGAIN;
            }
            tutil += util; // Accumulate GPU utilization
        }
    }
  }
  if (metric_ready) GPU_metric_ready_update(my_sig);
  /* We compute if all the gpus are done */
  uint total_ready = 0;
  for (i=0; i < my_sig->gpu_sig.num_gpus; i++) {
    total_ready += (GPU_policy_state[i] == PARTIAL_READY);
  }
  if (total_ready == my_sig->gpu_sig.num_gpus){
    for (i=0; i < my_sig->gpu_sig.num_gpus; i++) GPU_policy_state[i] = FIRST_SELECTION;
  }

  if (tutil > 0) {
        last_gpu_state = _GPU_Comp;
  }
  verbose_master(2,"%sGPU Optimize end ....%s %s", COL_BLU, optimize_criteria, COL_CLR);
#endif // USE_GPUS
  return EAR_SUCCESS;
}



state_t policy_restore_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
    ulong util, tutil = 0;;
    int i;

    /* WARNING: my_sig can be NULL */
    if (c == NULL) return EAR_ERROR;

#if USE_GPUS
    verbose_master(2, "Restoring GPU freqs: restore settings");
    if (my_sig == NULL){
      for (i = 0 ; i < c->num_gpus ; i++) {
        freqs->gpu_freq[i] = gfreqs[i];
      }
      return EAR_SUCCESS;
    }
    for (i=0;i<my_sig->gpu_sig.num_gpus;i++) {
        util = my_sig->gpu_sig.gpu_data[i].GPU_util;
        tutil += util;
        if (util == 0){
            verbose_master(2, "GPU[%d] freq %lu", i, gpuf_list[i][gpuf_list_items[i]-1]);
            freqs->gpu_freq[i] = gpuf_list[i][gpuf_list_items[i]-1];
        }else{
            verbose_master(2, "GPU[%d] freq %lu", i, gfreqs[i]);
            freqs->gpu_freq[i] = gfreqs[i];
        }
        GPU_policy_state[i] = FIRST_SELECTION;
        memset(last_GPU_pstate, 0, sizeof(my_sig->gpu_sig.num_gpus)*sizeof(uint));
    }
    if (tutil > 0) {
        last_gpu_state = _GPU_Comp;
    }

#endif // USE_GPUS
    return EAR_SUCCESS;
}

state_t policy_cpu_gpu_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
  verbose_master(2,"GPU policy_cpu_gpu_settings");
  return policy_apply(c, my_sig, freqs, (int *)&gpu_ready);
}

state_t policy_io_settings(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs)
{
  verbose_master(2,"GPU policy_io_settings");
  return policy_apply(c, my_sig, freqs, (int *)&gpu_ready);
}

state_t policy_busy_wait_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
  verbose_master(2,"GPU policy_busy_wait_settings");
  return policy_apply(c, my_sig, freqs, (int *)&gpu_ready);
}

