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

#define _GNU_SOURCE
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <metrics/imcfreq/archs/amd17.h>
	
static const char *mgt_names[] = { "mgt_imcfreq_load",
	                               "mgt_imcfreq_get_api",
		                           "mgt_imcfreq_init",
	                               "mgt_imcfreq_dispose",
	                               "mgt_imcfreq_count_devices", 
	                               "mgt_imcfreq_get_current_list" };

static struct mgt_ops_s {
	state_t (*load)             (topology_t *c, int eard);
	state_t (*get_api)          (uint *api);
	state_t (*init)             (ctx_t *c);
	state_t (*dispose)          (ctx_t *c);
	state_t (*count_devices)    (ctx_t *c, uint *devs_count);
	state_t (*get_current_list) (ctx_t *c, pstate_t *pstate_list);
} mgt_ops;

static imcfreq_ops_t *ops;
static uint devs_count;

void imcfreq_amd17_load(topology_t *tp, imcfreq_ops_t *ops_in, int eard)
{
	uint api_intern, api;

	debug("loading");

	ops = ops_in;	
	// Set BYPASS if:
	// 	- If !EARD and no other API is set yet.
	// Set BYPASS limited if:
	//  - If DAEMON and EARD API has been set as AMD17.
	if (apis_loaded(ops)) {
		debug("the API was already loaded");
		ops->get_api(&api, &api_intern);
		//
		if (api == API_EARD && api_intern == API_AMD17) {
			apis_add(ops->init_static, imcfreq_amd17_init_static);
	    	apis_set(ops->data_diff,   imcfreq_amd17_data_diff);
			debug("AMD17 loaded limited");
		}
		return;
	}
	debug("the API was not loaded yet");
	if (apis_not(ops)) {
		debug("1");
		// Loading symbols in runtime to avoid dependancies
		symplug_join(RTLD_DEFAULT, (void **) &mgt_ops, mgt_names, 6);
		debug("2");
		// Testing all symbols are available
		if (state_fail(symplug_test((void **) &mgt_ops, 6))) {
			return;
		}
		debug("3");
		// Loading management imcfreq API
		mgt_ops.load(tp, eard);
		mgt_ops.get_api(&api);
		debug("4");
		// If management API is not cool, bye
		if (api <= API_DUMMY) {
			return;
		}
	}
	// Bypassing
	apis_set(ops->get_api,       imcfreq_amd17_get_api);
	apis_set(ops->init,          imcfreq_amd17_init);
	apis_set(ops->dispose,       imcfreq_amd17_dispose);
	apis_set(ops->count_devices, imcfreq_amd17_count_devices);
	apis_set(ops->read,          imcfreq_amd17_read);
	apis_set(ops->data_diff,     imcfreq_amd17_data_diff);
	debug("AMD17 loaded full API");
}

void imcfreq_amd17_get_api(uint *api, uint *api_intern)
{
	*api = API_AMD17;
	if (api_intern) {
		*api_intern = API_NONE;
	}
}

state_t imcfreq_amd17_init(ctx_t *c)
{
	state_t s;

	if (state_fail(s = mgt_ops.init(c))) {
		return s;
	}
	return mgt_ops.count_devices(c, &devs_count);
}

state_t imcfreq_amd17_init_static(ctx_t *c)
{	
	return ops->count_devices(c, &devs_count);
}

state_t imcfreq_amd17_dispose(ctx_t *c)
{
	return mgt_ops.dispose(c);
}

state_t imcfreq_amd17_count_devices(ctx_t *c, uint *devs_count_in)
{
	*devs_count_in = devs_count;
	return EAR_SUCCESS;
}

state_t imcfreq_amd17_read(ctx_t *c, imcfreq_t *imc)
{
	pstate_t list[8]; // Up to 8 sockets
	state_t s;
	int cpu;

	if (state_fail(s = mgt_ops.get_current_list(c, list))) {
		return s;
	}
    timestamp_getfast(&imc[0].time);
    // Iterating per socket.
    for (cpu = 0; cpu < devs_count; ++cpu) {
		debug("AMD17 read %llu", list[cpu].khz);
        imc[cpu].time  = imc[0].time;
		imc[cpu].freq  = (ulong) list[cpu].khz;
		imc[cpu].error = (list[cpu].khz == 0LLU);
	}
	return EAR_SUCCESS;
}

state_t imcfreq_amd17_data_diff(imcfreq_t *i2, imcfreq_t *i1, ulong *freq_list, ulong *average)
{
	ulong aux1 = 0;
	ulong aux2 = 0;
    uint cpu;
	for (cpu = 0; cpu < devs_count; ++cpu) {
		//aux1 = (i2[cpu].freq + i1[cpu].freq) / 2LU;
		aux1 = i2[cpu].freq;
		if (freq_list != NULL) {
			freq_list[cpu] = aux1;
		}
		aux2 += aux1;
	}
	if (average != NULL) {
		*average = aux2;
		if (devs_count > 0) {
			*average = *average / (ulong) devs_count;
		}
	}
	return EAR_SUCCESS;
}
