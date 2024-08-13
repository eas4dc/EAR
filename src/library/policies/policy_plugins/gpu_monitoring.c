/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/



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
#include <management/gpu/gpu.h>
#include <library/policies/policy_api.h>
#include <library/policies/policy_state.h>
#include <library/metrics/metrics.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>

// extern uint cpu_ready, gpu_ready;

/* Wrap verbosing policy general info.
 * Currently, it is a verbose master with level 2. */
#define verbose_gpumon_info(msg, ...) \
	verbose_info2_master("[GPU monitoring] " msg, ##__VA_ARGS__);


static const ulong **gpuf_list;
static const uint *gpuf_list_items;
static  ulong *gfreqs;

state_t policy_init(polctx_t *c)
{
		/* Read the default GPU frequency to be set by an
		 * authorized user through an environment variable. */
    char *gpu_freq = ear_getenv(FLAG_GPU_DEF_FREQ);
    ulong g_freq = 0;
    if (gpu_freq && c->app->user_type == AUTHORIZED)
		{
        g_freq = atol(gpu_freq);
    }

    debug("Asking for frequencies");
    gpuf_list       = (const ulong **) metrics_gpus_get(MGT_GPU)->avail_list;
    gpuf_list_items = (const uint *)   metrics_gpus_get(MGT_GPU)->avail_count;

	/* MGT_GPU API might be DUMMY, so device count can be different with respect to 
	 * c->num_gpus (value taken from MET_GPU, which most of times is well loaded). */
	uint gpu_count = metrics_gpus_get(MGT_GPU)->devs_count;
	verbose_master(2, "%d GPUs detected, assuming all the GPUs have the same list of freqs we show only 1", gpu_count);

    for (int i = 0; i < ear_min(gpu_count,1); i++)
	{
        verbose_master(2, "Frequency list in GPU %d (%u items)", i, gpuf_list_items[i]);
        for (int j = 0; j < gpuf_list_items[i]; j++)
		{
			float gpu_freq_ghz = (float) gpuf_list[i][j] / 1000000.0;
			verbose_master(2, "GPU%d[%d]: %.2f (%lu kHz)", i, j, gpu_freq_ghz, gpuf_list[i][j]);
		}
    }

    mgt_gpu_data_alloc(&gfreqs);

    for (int i = 0; i < gpu_count; i++)
		{
			if (g_freq) gfreqs[i] = g_freq;
			else        gfreqs[i] = gpuf_list[i][0];
			verbose_master(2, "Setting GPUfreq[%d] = %.2f", i, (float) gfreqs[i] / 1000000.0);
    }

    mgt_gpu_freq_limit_set(no_ctx, gfreqs);

    verbose_gpumon_info("Plug-in loaded.");
    return EAR_SUCCESS;
}

state_t policy_apply(polctx_t *c, signature_t *my_sig, node_freqs_t *freqs, int *ready)
{
    ulong *new_freq=freqs->gpu_freq;
    uint i;
#if USE_GPUS
    debug("apply: num_gpus %d",my_sig->gpu_sig.num_gpus);
    for (i = 0; i < my_sig->gpu_sig.num_gpus; i++) {
        debug("[GPU %d Power %.2lf Freq %.2f Mem_freq %.2f Util %lu Mem_util %lu]",
                i, my_sig->gpu_sig.gpu_data[i].GPU_power,
                (float) my_sig->gpu_sig.gpu_data[i].GPU_freq / 1000.0,
                (float) my_sig->gpu_sig.gpu_data[i].GPU_mem_freq/1000.0,
                my_sig->gpu_sig.gpu_data[i].GPU_util,
                my_sig->gpu_sig.gpu_data[i].GPU_mem_util);
    }
#endif

    *ready = EAR_POLICY_READY;
    memcpy(new_freq, gfreqs, sizeof(ulong) * c->num_gpus);

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
  ulong *new_freq=freq_set->gpu_freq;
  memcpy(new_freq,gfreqs,sizeof(ulong)*c->num_gpus);
	return EAR_SUCCESS;
}

state_t policy_max_tries(polctx_t *c,int *intents)
{
	*intents=0;
	return EAR_SUCCESS;
}


