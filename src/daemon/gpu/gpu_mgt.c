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
#include <common/config.h>
#include <common/states.h>

#include <management/gpu/gpu.h>
//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/output/debug.h>

static ctx_t gpu_node_mgr;
static uint num_dev;
static ulong *def_khz,*max_khz,*current_khz;
static ulong *def_w,*max_w,*current_w,*min_w;
state_t gpu_mgr_init()
{
	uint i;
	state_t ret;
	ret=mgt_gpu_load(NULL);
	if (ret!=EAR_SUCCESS){
		debug("Error in mgt_gpu_load");
		return ret;
	}
	ret=mgt_gpu_init(&gpu_node_mgr);
	if (ret!=EAR_SUCCESS){
		debug("Error in mgt_gpu_init");
		return ret;
	}
	ret=mgt_gpu_count(&gpu_node_mgr,&num_dev);
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
	ret=mgt_gpu_freq_limit_get_default(&gpu_node_mgr,def_khz);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_freq_limit_get_default");
		return ret;
	}
	ret=mgt_gpu_freq_limit_get_max(&gpu_node_mgr,max_khz);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_freq_limit_get_max");
		return ret;
	}
	ret=mgt_gpu_freq_limit_get_current(&gpu_node_mgr,current_khz);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_freq_limit_get_current");
		return ret;
	}
	/* GPU power limits */
	ret=mgt_gpu_power_cap_get_default(&gpu_node_mgr,def_w);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_power_cap_limit_get_default");
		return ret;
	}
	ret=mgt_gpu_power_cap_get_rank(&gpu_node_mgr,min_w,max_w);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_power_cap_get_rank");
		return ret;
	}
	ret=mgt_gpu_power_cap_get_current(&gpu_node_mgr,current_w);
	if (ret!=EAR_SUCCESS){
		debug("mgt_gpu_power_cap_limit_get_current");
		return ret;
	}
	for (i=0;i<num_dev;i++){
		debug("GPU %u freq limits: def %lu max %lu current %lu",i,def_khz[i],max_khz[i],current_khz[i]);
		debug("GPU %u power limits:def %lu min %lu max %lu current %lu",i,def_w[i],min_w[i],max_w[i],current_w[i]);
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
	return mgt_gpu_freq_limit_set(&gpu_node_mgr,freqs);
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
  ret=mgt_gpu_freq_limit_set(&gpu_node_mgr,nfreq);
	free(nfreq);
	return ret;
}

uint    gpu_mgr_num_gpus()
{
	return num_dev;
}


