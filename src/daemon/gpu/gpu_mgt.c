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
#include <common/config.h>
#include <common/states.h>

#if USE_GPUS
#include <management/gpu/gpu.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <common/types/configuration/cluster_conf.h>

static uint num_dev;
static ulong *def_khz,*max_khz,*current_khz;
static ulong *def_w,*max_w,*current_w,*min_w;
static metrics_gpus_t mgt_gpu;
extern my_node_conf_t *my_node_conf;
state_t gpu_mgr_init()
{
	uint i;
	state_t ret;
	mgt_gpu_load(NO_EARD);
	ret=mgt_gpu_init(no_ctx);
	if (ret!=EAR_SUCCESS){
		debug("Error in mgt_gpu_init");
		return ret;
	}
	ret=mgt_gpu_count_devices(no_ctx,&num_dev);
	if (ret!=EAR_SUCCESS){
		debug("Error in mgt_gpu_count");
		return ret;
	}
	debug("%u gpus in node",num_dev);
	def_khz=calloc(num_dev,sizeof(ulong));
	max_khz=calloc(num_dev,sizeof(ulong));
	current_khz=calloc(num_dev,sizeof(ulong));
	def_w=calloc(num_dev,sizeof(ulong));
	max_w=calloc(num_dev,sizeof(ulong));
	min_w=calloc(num_dev,sizeof(ulong));
	current_w=calloc(num_dev,sizeof(ulong));
	if ((def_khz == NULL) || (max_khz == NULL) || (current_khz == NULL)){
		debug("Memory allocation for GPU freq limits returns NULL");
		return EAR_ALLOC_ERROR;
	}
	if ((def_w == NULL) || (max_w == NULL) || (current_w == NULL)){
		debug("Memory allocation for GPU power limits returns NULL");
		return EAR_ALLOC_ERROR;
	}
	/* GPU frequency limits */
	ret=mgt_gpu_freq_limit_get_default(no_ctx,def_khz);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_freq_limit_get_default");
		return ret;
	}
	ret=mgt_gpu_freq_limit_get_max(no_ctx,max_khz);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_freq_limit_get_max");
		return ret;
	}
	ret=mgt_gpu_freq_limit_get_current(no_ctx,current_khz);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_freq_limit_get_current");
		return ret;
	}
	/* GPU power limits */
	ret=mgt_gpu_power_cap_get_default(no_ctx,def_w);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_power_cap_limit_get_default");
		return ret;
	}
	ret=mgt_gpu_power_cap_get_rank(no_ctx,min_w,max_w);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_power_cap_get_rank");
		return ret;
	}
	ret=mgt_gpu_power_cap_get_current(no_ctx,current_w);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_power_cap_limit_get_current");
		return ret;
	}
	for (i=0;i<num_dev;i++){
		debug("GPU %u freq limits: def %lu max %lu current %lu",i,def_khz[i],max_khz[i],current_khz[i]);
		debug("GPU %u power limits:def %lu min %lu max %lu current %lu",i,def_w[i],min_w[i],max_w[i],current_w[i]);
	}
    /* If gpu_def_freq == 0 --> GPU is not set at job init/end
     * If gpu_def_freq > max GPU freq --> gpu_def_freq <- max GPU freq
     * else --> not modified
     * if gpu_def_freq > 0 --> GPU frequency will be set at job init/end */
	if (my_node_conf->gpu_def_freq > 0)
    {
		my_node_conf->gpu_def_freq = ear_min(my_node_conf->gpu_def_freq, max_khz[0]);
        verbose(VCONF, "Configuring GPU default frequency at job init/end to %lu",
                my_node_conf->gpu_def_freq);
	} else
    {
        verbose(VCONF, "Configuring GPU default frequency at job init/end DISABLED");
    }
  ret = mgt_gpu_freq_list(no_ctx, (const ulong ***) &mgt_gpu.avail_list, (const uint **) &mgt_gpu.avail_count);
  if (ret != EAR_SUCCESS){
    debug("mgt_gpu_freq_list");
    return ret;
  }

	return EAR_SUCCESS;
}

state_t gpu_mgr_set_freq(uint num_dev_req,ulong *freqs)
{
	int i;
	/* debug("Setting the GPU frequency"); */
	if (num_dev_req>num_dev){
		error("Num GPUS requestedi(%u)  to change frequency greather than num GPUS detected (%u)",num_dev_req,num_dev);
		num_dev_req=num_dev;
	}
	for (i=0;i<num_dev_req;i++){
		freqs[i]=freqs[i];
		debug("gpu_mgr_set_freq gpu[%d]=%lu",i,freqs[i]);
	}
	return mgt_gpu_freq_limit_set(no_ctx,freqs);
}

state_t gpu_mgr_set_freq_all_gpus(ulong gfreq)
{
	int i;
	state_t ret;
	debug("Setting the GPU frequency %lu to all gpus",gfreq);
	ulong *nfreq=calloc(num_dev,sizeof(ulong));	
	for (i=0;i<num_dev;i++){
		nfreq[i]=gfreq;
  }
  ret=mgt_gpu_freq_limit_set(no_ctx,nfreq);
	free(nfreq);
	return ret;
}

uint    gpu_mgr_num_gpus()
{
	return num_dev;
}

state_t gpu_mgr_get_min(uint gpu, ulong *gfreq)
{
  if (gpu > num_dev) return EAR_ERROR;
  ulong *list = mgt_gpu.avail_list[gpu];
  *gfreq = list[mgt_gpu.avail_count[gpu] - 1];
  return EAR_SUCCESS;
}

state_t gpu_mgr_get_max(uint gpu, ulong *gfreq)
{
  if (gpu > num_dev) return EAR_ERROR;
  *gfreq = max_khz[gpu];
  return EAR_SUCCESS;
}
#endif
