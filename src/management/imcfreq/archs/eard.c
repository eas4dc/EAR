/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <common/output/debug.h>
#include <common/utils/serial_buffer.h>
#include <management/imcfreq/archs/eard.h>
#include <daemon/local_api/eard_api.h>

#define RPC_GET_API       RPC_MGT_IMCFREQ_GET_API
#define RPC_GET_AVAIL_PS  RPC_MGT_IMCFREQ_GET_AVAILABLE
#define RPC_GET_CURR_PS   RPC_MGT_IMCFREQ_GET_CURRENT
#define RPC_SET_CURR_PS   RPC_MGT_IMCFREQ_SET_CURRENT
#define RPC_GET_CURR_PSM  RPC_MGT_IMCFREQ_GET_CURRENT_MIN
#define RPC_SET_CURR_PSM  RPC_MGT_IMCFREQ_SET_CURRENT_MIN

static uint       sockets_count;
static pstate_t  *available_copy;
static pstate_t  *available_list;
static uint       available_count;
static wide_buffer_t w;

state_t mgt_imcfreq_eard_load(topology_t *tp, mgt_imcfreq_ops_t *ops, int eard)
{
	static wide_buffer_t b;
	int eard_api;
	state_t s;

	if (!eard) {
		debug("Error: EARD (daemon) not required");
		return_msg(EAR_ERROR, "EARD (daemon) not required");
	}
	if (!eards_connected()) {
		debug("Error: EARD (daemon) not connected");
		return_msg(EAR_ERROR, "EARD (daemon) not connected");
	}
    serial_alloc(&w, SIZE_2KB);
	if (state_fail(s = eard_rpc(RPC_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
		debug("Error: Sending RPC_GET_API");
		return s;
	}
	debug("Received API %u", eard_api);
	if (eard_api == API_NONE || eard_api == API_DUMMY) {
		return_msg(EAR_ERROR, "EARD (daemon) has loaded DUMMY API");
	}
	// Buffered function returns a pointer to the internal serial/wide buffer
	if (state_fail(s = eard_rpc_buffered(RPC_GET_AVAIL_PS, NULL, 0, (char **) &b, NULL))) {
		return s;
	}
	// Filling lists
	serial_copy_elem(&b, (char *) &available_count, NULL);
	available_list = calloc(available_count, sizeof(pstate_t));
	available_copy = calloc(available_count, sizeof(pstate_t));
	serial_copy_elem(&b, (char *) available_list, NULL);

	#if SHOW_DEBUGS
	debug("Available %u IMC frequencies:", available_count);
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

// eards_mgt_imcfreq_get(UNC_INIT_DATA
// eards_mgt_imcfreq_get(UNC_GET_LIMITS,
// eards_mgt_imcfreq_set(UNC_SET_LIMITS,
// eards_mgt_imcfreq_get(UNC_GET_LIMITS_MIN,
// eards_mgt_imcfreq_set(UNC_SET_LIMITS_MIN,
//

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
	return eard_rpc(RPC_GET_CURR_PS, NULL, 0, (char *) pstate_list, size);
}

state_t mgt_imcfreq_eard_set_current_list(ctx_t *c, uint *index_list)
{
	size_t size = sizeof(uint) * sockets_count;
	return eard_rpc(RPC_SET_CURR_PS, (char *) index_list, size, NULL, 0);
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
	static wide_buffer_t b;
	state_t s;

    memset(ps_min_list, 0, sizeof(pstate_t)*sockets_count);
    memset(ps_max_list, 0, sizeof(pstate_t)*sockets_count);
	if (state_fail(s = eard_rpc_buffered(RPC_GET_CURR_PSM, NULL, 0, (char **) &b, NULL))) {
		return s;
	}
	serial_copy_elem(&b, (char *) ps_min_list, NULL);
	serial_copy_elem(&b, (char *) ps_max_list, NULL);
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_eard_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list)
{
    #if SHOW_DEBUGS
    int i;
    for (i = 0; i < sockets_count; ++i) {
        debug("IMC%d: setting frequency from %u to %u",
            i, id_min_list[i], id_max_list[i]);	
    }
    #endif

    serial_clean(&w);
	serial_add_elem(&w, (char *) id_min_list, sizeof(uint)*sockets_count);
	serial_add_elem(&w, (char *) id_max_list, sizeof(uint)*sockets_count);
	return eard_rpc(RPC_SET_CURR_PSM, serial_data(&w), serial_size(&w), NULL, 0);
}
