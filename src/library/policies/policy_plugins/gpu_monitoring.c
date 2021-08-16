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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
//#define SHOW_DEBUGS 1
#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <library/policies/policy_api.h>
#include <library/policies/policy_state.h>
#include <library/metrics/gpu.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>

extern uint cpu_ready, gpu_ready, gidle;


static const ulong **gpuf_list;
static const uint *gpuf_list_items;
static  ulong *gfreqs;
state_t policy_init(polctx_t *c)
{
  int i,j;
  ulong g_freq = 0;
  char *gpu_freq=getenv(SCHED_EAR_GPU_DEF_FREQ);
  if ((gpu_freq!=NULL) && (c->app->user_type==AUTHORIZED)){
    g_freq = atol(gpu_freq);
  }
  debug("Asking for frequencies");
  gpu_lib_freq_list(&gpuf_list, &gpuf_list_items);
  for (i=0;i<c->num_gpus;i++){
    verbose_master(2,"Freqs in GPU[%d]=%u",i,gpuf_list_items[i]);
    for (j=0;j<gpuf_list_items[i];j++){
      verbose_master(2,"GPU[%d][%d]=%.2f",i,j,(float)gpuf_list[i][j]/1000000.0);
    }
  }
  gpu_lib_alloc_array(&gfreqs);
  for (i=0;i<c->num_gpus;i++){
    if (g_freq) gfreqs[i] = g_freq;
    else        gfreqs[i] = gpuf_list[i][0];
    verbose_master(2,"Setting GPUgreq[%d] = %.2f",i,(float)gfreqs[i]/1000000.0);
  }
  gpu_lib_freq_limit_set(gfreqs);
	verbose_master(2,"GPU monitoring loaded");
  return EAR_SUCCESS;
}
state_t policy_apply(polctx_t *c,signature_t *my_sig, node_freqs_t *freqs,int *ready)
{
	ulong *new_freq=freqs->gpu_freq;
	uint i;
	#if USE_GPUS
	debug("apply: num_gpus %d",my_sig->gpu_sig.num_gpus);
	for (i=0;i<my_sig->gpu_sig.num_gpus;i++){
		debug("[GPU %d Power %.2lf Freq %.2f Mem_freq %.2f Util %lu Mem_util %lu]",i,
		my_sig->gpu_sig.gpu_data[i].GPU_power,(float)my_sig->gpu_sig.gpu_data[i].GPU_freq/1000.0,(float)my_sig->gpu_sig.gpu_data[i].GPU_mem_freq/1000.0,
		my_sig->gpu_sig.gpu_data[i].GPU_util,my_sig->gpu_sig.gpu_data[i].GPU_mem_util);
	}
	#endif
	
	*ready = EAR_POLICY_READY;
	memcpy(new_freq,gfreqs,sizeof(ulong)*c->num_gpus);
	
	return EAR_SUCCESS;
}
state_t policy_ok(polctx_t *c, signature_t *my_sig,signature_t *prev_sig,int *ok)
{
	uint i;
	#if USE_GPUS
	debug("num_gpus %d",my_sig->gpu_sig.num_gpus);
	for (i=0;i<my_sig->gpu_sig.num_gpus;i++){
		debug("[GPU %d Power %.2lf Freq %.2f Mem_freq %.2f Util %lu Mem_util %lu]",i,
		my_sig->gpu_sig.gpu_data[i].GPU_power,(float)my_sig->gpu_sig.gpu_data[i].GPU_freq/1000.0,(float)my_sig->gpu_sig.gpu_data[i].GPU_mem_freq/1000.0,
		my_sig->gpu_sig.gpu_data[i].GPU_util,my_sig->gpu_sig.gpu_data[i].GPU_mem_util);
	}
	#endif
	*ok=1;

	return EAR_SUCCESS;
}


state_t policy_get_default_freq(polctx_t *c, node_freqs_t *freq_set,signature_t *s)
{
	return EAR_SUCCESS;
}

state_t policy_max_tries(polctx_t *c,int *intents)
{
	*intents=0;
	return EAR_SUCCESS;
}


