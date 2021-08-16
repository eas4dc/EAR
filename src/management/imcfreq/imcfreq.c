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

#include <stdio.h>
#include <pthread.h>
#include <metrics/common/msr.h>
#include <common/output/debug.h>
#include <management/imcfreq/imcfreq.h>
#include <management/imcfreq/archs/eard.h>
#include <management/imcfreq/archs/dummy.h>
#include <management/imcfreq/archs/amd17.h>
#include <management/imcfreq/archs/intel63.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static mgt_imcfreq_ops_t ops;
static uint sockets_count;
static uint api;

state_t mgt_imcfreq_load(topology_t *tp, int eard)
{
	state_t s;
	while (pthread_mutex_trylock(&lock));
	debug("loading");
	if (api != API_NONE) {
		debug("API already loaded");
		pthread_mutex_unlock(&lock);
		return EAR_SUCCESS;
	}
	sockets_count = tp->socket_count;
	// API load
	api = API_DUMMY;
	if (state_ok(s = mgt_imcfreq_amd17_load(tp, &ops))) {
		api = API_AMD17;
		debug("Loaded AMD17");
	}
	if (state_ok(s = mgt_imcfreq_intel63_load(tp, &ops))) {
		api = API_INTEL63;
		debug("Loaded INTEL63");
	}
	if (state_ok(s = mgt_imcfreq_eard_load(tp, &ops, eard))) {
		api = API_EARD;
		debug("Loaded EARD");
	}
	mgt_imcfreq_dummy_load(tp, &ops);
	pthread_mutex_unlock(&lock);
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_get_api(uint *api_in)
{
	*api_in = api;
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_init(ctx_t *c)
{
	preturn (ops.init, c);
}

state_t mgt_imcfreq_dispose(ctx_t *c)
{
	preturn (ops.dispose, c);
}

state_t mgt_imcfreq_count_devices(ctx_t *c, uint *devs_count)
{
	preturn (ops.count_devices, c, devs_count);
}

state_t mgt_imcfreq_get_available_list(ctx_t *c, const pstate_t **pstate_list, uint *pstate_count)
{
	preturn (ops.get_available_list, c, pstate_list, pstate_count);
}

state_t mgt_imcfreq_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	preturn (ops.get_current_list, c, pstate_list);
}

state_t mgt_imcfreq_set_current_list(ctx_t *c, uint *index_list)
{
	preturn (ops.set_current_list, c, index_list);
}

state_t mgt_imcfreq_set_current(ctx_t *c, uint pstate_index, int socket)
{
	preturn (ops.set_current, c, pstate_index, socket);
}

state_t mgt_imcfreq_set_auto(ctx_t *c)
{
	preturn (ops.set_auto, c);
}

/** */
state_t mgt_imcfreq_get_current_ranged_list(ctx_t *c, pstate_t *ps_min_list, pstate_t *ps_max_list)
{
	preturn (ops.get_current_ranged_list, c, ps_min_list, ps_max_list);
}

state_t mgt_imcfreq_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list)
{
	preturn (ops.set_current_ranged_list, c, id_min_list, id_max_list);
}

/** */
state_t mgt_imcfreq_data_alloc(pstate_t **pstate_list, uint **index_list)
{
	preturn (ops.data_alloc, pstate_list, index_list);
}

void mgt_imcfreq_data_print(pstate_t *ps_list, uint ps_count, int fd)
{
	pstate_print(ps_list, ps_count, fd);
}

char *mgt_imcfreq_data_tostr(pstate_t *ps_list, uint ps_count, char *buffer, int length)
{
	return pstate_tostr(ps_list, ps_count, buffer, length);
}
