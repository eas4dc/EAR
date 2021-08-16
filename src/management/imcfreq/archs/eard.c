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

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/utils/serial_buffer.h>
#include <management/imcfreq/archs/eard.h>
#include <daemon/local_api/eard_api.h>
#include <daemon/local_api/eard_conf_api.h>

static uint       sockets_count;
static pstate_t  *available_copy;
static pstate_t  *available_list;
static uint       available_count;

state_t mgt_imcfreq_eard_load(topology_t *tp, mgt_imcfreq_ops_t *ops, int eard)
{
	char buffer[4096];
	int eard_api;
	state_t s;

	if (!eard) {
		return_msg(EAR_ERROR, "EARD (daemon) not required");
	}
	if (!eards_connected()) {
		return_msg(EAR_ERROR, "EARD (daemon) not connected");
	}
	//
	if (state_fail(s = eards_mgt_imcfreq_get(UNC_INIT_DATA, buffer, 4096))) {
		return s;
	}
	eard_api        = (uint) buffer[0];
	available_count = (uint) buffer[4];
	available_list  = calloc(available_count, sizeof(pstate_t)); 
	available_copy  = calloc(available_count, sizeof(pstate_t));
	memcpy(available_list, &buffer[8], available_count * sizeof(pstate_t));
	//
	debug("Received API %u", eard_api);
	if (eard_api == API_NONE || eard_api == API_DUMMY) {
        return_msg(EAR_ERROR, "EARD (daemon) has loaded DUMMY API");
    }
	debug("Available memory controller frequencies:");
	#if SHOW_DEBUGS
	int i;
	for (i = 0; i < available_count; ++i) {
		debug("PS%d: %llu KHz", i, available_list[i].khz);
	}
	#endif
	//
	replace_ops(ops->init,               mgt_imcfreq_eard_init);
	replace_ops(ops->dispose,            mgt_imcfreq_eard_dispose);
	replace_ops(ops->get_available_list, mgt_imcfreq_eard_get_available_list);
	replace_ops(ops->get_current_list,   mgt_imcfreq_eard_get_current_list);
	replace_ops(ops->set_current_list,   mgt_imcfreq_eard_set_current_list);
	replace_ops(ops->set_current,        mgt_imcfreq_eard_set_current);
	replace_ops(ops->set_auto,           mgt_imcfreq_eard_set_auto);
	replace_ops(ops->get_current_ranged_list, mgt_imcfreq_eard_get_current_ranged_list);
	replace_ops(ops->set_current_ranged_list, mgt_imcfreq_eard_set_current_ranged_list);
	//
    sockets_count  = tp->socket_count;
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_eard_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_eard_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_eard_get_available_list(ctx_t *c, const pstate_t **list, uint *count)
{
	uint i;
	for (i = 0; i < available_count; ++i) {
		available_copy[i].khz = available_list[i].khz;
		available_copy[i].idx = i;
	}
	*list = available_copy;
	if (count != NULL) {
		*count = available_count;
	}
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_eard_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	size_t size = sizeof(pstate_t) * sockets_count;
	return eards_mgt_imcfreq_get(UNC_GET_LIMITS, (char *) pstate_list, size);
}

state_t mgt_imcfreq_eard_set_current_list(ctx_t *c, uint *index_list)
{
	return eards_mgt_imcfreq_set(UNC_SET_LIMITS, index_list, sockets_count);
}

state_t mgt_imcfreq_eard_set_current(ctx_t *c, uint pstate_index, int socket)
{
	uint buffer[128];
	int i;
	for (i = 0; i < sockets_count; ++i) {
		buffer[i] = ps_nothing;
		if (i != socket && socket != all_sockets) {
            continue;
        }
		buffer[i] = pstate_index;
	}
	return mgt_imcfreq_eard_set_current_list(c, buffer);
}

state_t mgt_imcfreq_eard_set_auto(ctx_t *c)
{
	return mgt_imcfreq_eard_set_current(c, ps_auto, all_sockets);
}

state_t mgt_imcfreq_eard_get_current_ranged_list(ctx_t *c, pstate_t *ps_min_list, pstate_t *ps_max_list)
{
	size_t size = sizeof(pstate_t) * sockets_count;
	char buffer[1024];
	state_t s;

	if (state_fail(s = eards_mgt_imcfreq_get(UNC_GET_LIMITS_MIN, buffer, size*2))) {
		return s;
	}
	memcpy((char *) ps_min_list, &buffer[0]   , size);
	memcpy((char *) ps_max_list, &buffer[size], size);
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_eard_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list)
{
	size_t size = sizeof(uint) * sockets_count;
	char buffer[1024];
	memcpy(&buffer[0]   , (char *) id_min_list, size);
	memcpy(&buffer[size], (char *) id_max_list, size);
	return eards_mgt_imcfreq_set(UNC_SET_LIMITS_MIN, (uint *) buffer, sockets_count*2);
}
