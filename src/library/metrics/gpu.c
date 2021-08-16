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

//#define SHOW_DEBUGS 1
#include <common/output/verbose.h>
#include <common/output/debug.h>
#include <library/metrics/gpu.h>
#include <library/common/externs.h>
#include <library/common/verbose_lib.h>
#include <daemon/local_api/eard_api.h>

typedef struct ctx_gpu_s {
	ctx_t ctx_met;
	ctx_t ctx_man;
} ctx_gpu_t;

static uint			  dev_count, node_dev_count;
static gpu_ops_t     *ops_met;
static mgt_gpu_ops_t *ops_man;
static uint the_gpu_model = MODEL_UNDEFINED;
static gpu_t *node_gpu_data;
static ulong *node_gpu_freqs = NULL;
static uint *my_gpu_list;

state_t gpu_lib_load(settings_conf_t *settings)
{
	state_t ret;
	state_t s;
	if ((ret=eards_gpu_model(&the_gpu_model))!=EAR_SUCCESS) return ret;
	debug("eards_gpu_model %u ",the_gpu_model);
	//
	if (xtate_fail(s, gpu_load(&ops_met, none, empty))) {
		error("gpu_load returned %d (%s)", s, state_msg);
		return s;
	}
	if (xtate_fail(s, mgt_gpu_load(&ops_man))) {
		error("mgt_gpu_load returned %d (%s)", s, state_msg);
		return s;
	}
	if (the_gpu_model != gpu_model()){
		debug("GPU EARD model %u != EARL GPU model %u",the_gpu_model,gpu_model());
		the_gpu_model = gpu_model();
	}

	return EAR_SUCCESS;
}

state_t gpu_lib_init(ctx_t *_c)
{
	ctx_gpu_t *c = (ctx_gpu_t *) _c->context;
	state_t s;
	state_t ret;
	int i;
	if ((ret=eards_gpu_dev_count(&node_dev_count))!=EAR_SUCCESS) return ret;
	debug("eards_gpu_dev_count %u",node_dev_count);
	if (c == NULL) {
		c = calloc(1, sizeof(ctx_gpu_t));
		_c->context = (ctx_t *) c;
	}
	//
	debug("testing metrics unprivileged");
	if (xtate_fail(s, ops_met->init_unprivileged(&c->ctx_met))) {
		error("ops_met->init_unprivileged returned %d (%s)", s, state_msg);
		return s;
	}
	debug("testing mgt unprivileged");
	if (xtate_fail(s, ops_man->init_unprivileged(&c->ctx_man))) {
		error("ops_man->init_unprivileged returned %d (%s)", s, state_msg);
		return s;
	}
	if ((gpu_model() == MODEL_DUMMY) && (node_dev_count > 1)) dev_count = 1;
	else if (node_dev_count != dev_count)                     ops_man->count(&c->ctx_man,&dev_count);
	else                                                      dev_count = node_dev_count;
	debug("Using %d gpus from %d in node ", dev_count,node_dev_count);
	node_gpu_data  = calloc(node_dev_count,sizeof(gpu_t));
	node_gpu_freqs = calloc(node_dev_count,sizeof(ulong));
	my_gpu_list    = calloc(node_dev_count,sizeof(uint));
	if ((node_gpu_data == NULL) || (node_gpu_freqs == NULL)){
		error("Not enough memory");
		return EAR_ERROR;
	}
	/* This line must be replaces by the commented code */
	for (i=0;i<node_dev_count;i++) my_gpu_list[i] = 1;
	#if 0
	if (the_gpu_model != MODEL_DUMMY){
		gpu_info_t *info = malloc(sizeof(gpu_info_t)*node_dev_count);
		const gpu_info_t *local_info;
		if (gpu_get_info(&c->ctx_met,&local_info,NULL) != EAR_SUCCESS){
			verbose_master(2,"Error requesting local GPU info");
		}else{
			int i;
			for (i=0;i<dev_count; i++){
				verbose_master(2,"Local GPU[%d] serial: %llu", i, local_info[i].serial);
			}
		}
		eards_gpu_get_info((gpu_info_t *) info, node_dev_count);
		int i, li;
		for (i = 0; i < node_dev_count; ++i) {
			li = 0;
			while ((li < dev_count) && (info[i].serial != local_info[li].serial)) li++;
			if (li < dev_count){ 
				verbose_master(2,"Node GPU[%d] serial: %llu - Local GPU[%d]: %llu", i, info[i].serial,li,local_info[li].serial);
				my_gpu_list[i] = 1;
			}

		}
	}
	#endif

	return EAR_SUCCESS;
}

state_t gpu_lib_model(ctx_t *_c,uint *model)
{

	*model = the_gpu_model;
	
	return EAR_SUCCESS;
}

state_t gpu_lib_dispose(ctx_t *_c)
{
	ctx_gpu_t *c = (ctx_gpu_t *) _c->context;
	ops_met->dispose(&c->ctx_met);
	ops_man->dispose(&c->ctx_man);
	return EAR_SUCCESS;
}

state_t gpu_lib_count(uint *_dev_count)
{
	*_dev_count = dev_count;
	return EAR_SUCCESS;
}



/*
 *
 * Metrics
 *
 */
state_t gpu_lib_read(ctx_t *c, gpu_t *data)
{
	state_t ret;
	int i, li = 0;
	memset(node_gpu_data, 0 , node_dev_count*sizeof(gpu_t));
	if (the_gpu_model == MODEL_DUMMY){
		memset(data,0,dev_count*sizeof(gpu_t));
		return EAR_SUCCESS;
	}
	debug("reading %u gpus",node_dev_count);
	if ((ret=eards_gpu_data_read(node_gpu_data,node_dev_count))!=EAR_SUCCESS) return ret;	
	debug("Moving %d gpu data to user data",dev_count);
	for (i=0;i<node_dev_count;i++){
		if(my_gpu_list[i]){ 
			memcpy(&data[li],&node_gpu_data[i],sizeof(gpu_t));
			li++;
		}
	}
#if SHOW_DEBUGS
	char gpu_ste[256];
	gpu_lib_data_tostr(data,gpu_ste,sizeof(gpu_ste));
	debug("gpu data %s",gpu_ste);
#endif
	return EAR_SUCCESS;
}

state_t gpu_lib_read_raw(ctx_t *_c, gpu_t *data)
{
	ctx_gpu_t *c = (ctx_gpu_t *) _c->context;
	return ops_met->read_raw(&c->ctx_met, data);
}

state_t gpu_lib_read_copy(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	state_t s;
	if (xtate_fail(s, gpu_lib_read(NULL, data2))) {
		return s;
	}
	if (xtate_fail(s, ops_met->data_diff(data2, data1, data_diff))) {
		return s;
	}
	return ops_met->data_copy(data1, data2);
}

state_t gpu_lib_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	return ops_met->data_diff(data2, data1, data_diff);
}

state_t gpu_lib_data_merge(gpu_t *data_diff, gpu_t *data_merge)
{
	return ops_met->data_merge(data_diff, data_merge);
}

state_t gpu_lib_data_alloc(gpu_t **data)
{
	return ops_met->data_alloc(data);
}

state_t gpu_lib_data_free(gpu_t **data)
{
	return ops_met->data_free(data);
}

state_t gpu_lib_data_null(gpu_t *data)
{
	return ops_met->data_null(data);
}

state_t gpu_lib_data_copy(gpu_t *data_dst, gpu_t *data_src)
{
	return ops_met->data_copy(data_dst, data_src);
}

state_t gpu_lib_data_print(gpu_t *data, int fd)
{
	return ops_met->data_print(data, fd);
}

state_t gpu_lib_data_tostr(gpu_t *data, char *buffer, int length)
{
	return ops_met->data_tostr(data, buffer, length);
}

/*
 *
 * Management
 *
 */
state_t gpu_lib_alloc_array(ulong **array)
{
	return ops_man->alloc_array(NULL, array, NULL);
}

state_t gpu_lib_freq_limit_get_current(ctx_t *_c, ulong *khz)
{
	ctx_gpu_t *c = (ctx_gpu_t *) _c->context;
	return ops_man->freq_limit_get_current(&c->ctx_man, khz);
}

state_t gpu_lib_freq_limit_get_default(ulong *khz)
{
	return ops_man->freq_limit_get_default(NULL, khz);
}

state_t gpu_lib_freq_limit_get_max(ulong *khz)
{
	return ops_man->freq_limit_get_max(NULL, khz);
}

state_t gpu_lib_freq_limit_reset()
{
	// EARD
	return EAR_SUCCESS;
}

state_t gpu_lib_freq_limit_set(ulong *khz)
{
	int ret,i,li;
	if (the_gpu_model == MODEL_DUMMY){
		return EAR_SUCCESS;
	}
	if (khz == NULL){  
		state_return_msg(EAR_ERROR,EAR_BAD_ARGUMENT,"Invalid NULL argument");
	}
	for (i=0;i<dev_count;i++){
		verbose_master(2,"GPU_LIB_FREQ[%d]=%lu",i,khz[i]);
	}
	memset(node_gpu_freqs,0,sizeof(ulong)*node_dev_count);
	li = 0;
	for (i=0;i<node_dev_count;i++){
		if (my_gpu_list[i]){
			node_gpu_freqs[i] = khz[li];
			li++;
		}
	}
	ret = eards_gpu_set_freq(node_dev_count,node_gpu_freqs);
	if (ret != EAR_SUCCESS){
		state_return_msg(EAR_ERROR,EAR_SYSCALL_ERROR,"EARD local API error in GPU_Set_freq");
	}
	return EAR_SUCCESS;
}

state_t gpu_lib_freq_list(const ulong ***list_khz, const uint **list_len)
{
	return ops_man->freq_list(NULL, list_khz, list_len);
}

state_t gpu_lib_power_cap_get_current(ctx_t *_c, ulong *watts)
{
	ctx_gpu_t *c = (ctx_gpu_t *) _c->context;
	return ops_man->power_cap_get_current(&c->ctx_man, watts);
}

state_t gpu_lib_power_cap_get_default(ulong *watts)
{
	return ops_man->power_cap_get_default(NULL, watts);
}

state_t gpu_lib_power_cap_get_rank(ulong *watts_min, ulong *watts_max)
{
	return ops_man->power_cap_get_rank(NULL, watts_min, watts_max);
}

state_t gpu_lib_power_cap_reset()
{
	// EARD
	return EAR_SUCCESS;
}

state_t gpu_lib_power_cap_set(ulong *watts)
{
	// EARD
	return EAR_SUCCESS;
}


