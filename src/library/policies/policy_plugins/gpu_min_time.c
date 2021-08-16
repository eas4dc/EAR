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
#include <common/config.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <library/common/externs.h>
#include <library/policies/policy_api.h>
#include <library/policies/policy_state.h>
#include <library/common/verbose_lib.h>
#include <library/metrics/gpu.h>
#include <library/api/clasify.h>

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

static const ulong **gpuf_list;
static const uint *gpuf_list_items;
static	ulong *gfreqs;
extern uint last_earl_gpu_phase_classification;

state_t policy_init(polctx_t *c)
{
    int i,j;
    ulong g_freq = 0;
    char *gpu_freq=getenv(SCHED_EAR_GPU_DEF_FREQ);
    if ((gpu_freq!=NULL) && (c->app->user_type==AUTHORIZED)){
        g_freq=atol(gpu_freq);
    }
    gpu_lib_freq_list(&gpuf_list, &gpuf_list_items);
    for (i=0;i<c->num_gpus;i++){
        verbose_master(3,"Freqs in GPU[%d]=%u",i,gpuf_list_items[i]);
        for (j=0;j<gpuf_list_items[i];j++){
            verbose_master(3,"GPU[%d][%d]=%.2f",i,j,(float)gpuf_list[i][j]/1000000.0);
        }
    }
    gpu_lib_alloc_array(&gfreqs);
    for (i=0;i<c->num_gpus;i++){
        if (g_freq) gfreqs[i] = g_freq;
        else 				gfreqs[i] = gpuf_list[i][0];
        verbose_master(2,"Setting GPUgreq[%d] = %.2f",i,(float)gfreqs[i]/1000000.0);
    }
    gpu_lib_freq_limit_set(gfreqs);

    return EAR_SUCCESS;
}

state_t policy_apply(polctx_t *c,signature_t *my_sig, node_freqs_t *freqs,int *ready)
{
    ulong *new_freq=freqs->gpu_freq;
    uint i;
    ulong util, tutil = 0;;
    *ready = EAR_POLICY_READY;
#if USE_GPUS
#if SHOW_DEBUGS
    debug("apply: num_gpus %d",my_sig->gpu_sig.num_gpus);
    for (i=0;i<my_sig->gpu_sig.num_gpus;i++){
        debug("[GPU %d Power %.2lf Freq %.2f Mem_freq %.2f Util %lu Mem_util %lu]",i,
                my_sig->gpu_sig.gpu_data[i].GPU_power,(float)my_sig->gpu_sig.gpu_data[i].GPU_freq/1000.0,(float)my_sig->gpu_sig.gpu_data[i].GPU_mem_freq/1000.0,
                my_sig->gpu_sig.gpu_data[i].GPU_util,my_sig->gpu_sig.gpu_data[i].GPU_mem_util);
    }
#endif

    for (i=0;i<my_sig->gpu_sig.num_gpus;i++){
        util = my_sig->gpu_sig.gpu_data[i].GPU_util;
        tutil += util;
        if (util == 0){		
            new_freq[i] = gpuf_list[i][gpuf_list_items[i]-1];
        }else{
            new_freq[i] = gfreqs[i];
        }
    }		
    if ((tutil > 0 ) && (last_earl_gpu_phase_classification == GPU_IDLE)) last_earl_gpu_phase_classification = GPU_COMP;

#endif

    return EAR_SUCCESS;
}

state_t policy_restore_settings(polctx_t *c,signature_t *my_sig,node_freqs_t *freqs)
{
    ulong util, tutil = 0;;

    if (c == NULL) return EAR_ERROR;
    int i;
#if USE_GPUS
    verbose_master(2,"Restaurando GPU freqs: restore settings");
    for (i=0;i<my_sig->gpu_sig.num_gpus;i++){
        util = my_sig->gpu_sig.gpu_data[i].GPU_util;
        tutil += util;
        if (util == 0){
            freqs->gpu_freq[i] = gpuf_list[i][gpuf_list_items[i]-1];
        }else{
            freqs->gpu_freq[i] = gfreqs[i];
        }
    }
    if ((tutil > 0 ) && (last_earl_gpu_phase_classification == GPU_IDLE)) last_earl_gpu_phase_classification = GPU_COMP;

#endif
    return EAR_SUCCESS;
}

