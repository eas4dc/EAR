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

#include <common/output/debug.h>
#include <daemon/local_api/eard_api.h>
#include <metrics/imcfreq/archs/eard.h>

static uint dev_count;
static uint eard_api;

void imcfreq_eard_load(topology_t *tp, imcfreq_ops_t *ops, int eard)
{
    state_t s;

	debug("loading (eard: %d)", eard);
	if (!eard) {
		return;
	}
	debug("connecting with daemon");
	if (!eards_connected()) {
		debug("EARD is not running or EAR_TMP is not set correctly");
		return;
	}
	debug("asking data to daemon");
	// Contacting daemon
    if (state_fail(s = eard_rpc(RPC_MET_IMCFREQ_GET_API, NULL, 0, (char *) &eard_api, sizeof(uint)))) {
        debug("RPC RPC_MET_CPUFREQ_GET_API returned: %s (%d)", state_msg, s);
        return;
    }
    if (state_fail(s = eard_rpc(RPC_MET_IMCFREQ_COUNT_DEVICES, NULL, 0, (char *) &dev_count, sizeof(uint)))) {
        debug("RPC RPC_MET_IMCFREQ_COUNT_DEVICES returned: %s (%d)", state_msg, s);
        return;
    }
	//
	if (eard_api == API_NONE || eard_api == API_DUMMY) {
		debug("EARD has loaded DUMMY API");
		return;
	}
	if (!dev_count) {
		return;
	}
	apis_set(ops->get_api,       imcfreq_eard_get_api);
    apis_set(ops->init,          imcfreq_eard_init);
    apis_set(ops->dispose,       imcfreq_eard_dispose);
    apis_set(ops->count_devices, imcfreq_eard_count_devices);
    apis_set(ops->read,          imcfreq_eard_read);
	debug("EARD loaded full API");
}

void imcfreq_eard_get_api(uint *api, uint *api_intern)
{
    *api = API_EARD;
	if (api_intern) {
		*api_intern = eard_api;
	}
}

state_t imcfreq_eard_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t imcfreq_eard_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t imcfreq_eard_count_devices(ctx_t *c, uint *dev_count_in)
{
	*dev_count_in = dev_count;
	return EAR_SUCCESS;
}

state_t imcfreq_eard_read(ctx_t *c, imcfreq_t *list)
{
    memset((void *) list, 0, sizeof(imcfreq_t)*dev_count);
    return eard_rpc(RPC_MET_IMCFREQ_GET_CURRENT, NULL, 0, (char *) list, sizeof(imcfreq_t)*dev_count);
}
