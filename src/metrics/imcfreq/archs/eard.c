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

#include <common/output/debug.h>
#include <daemon/local_api/eard_api.h>
#include <metrics/imcfreq/archs/eard.h>

static uint eard_api;
static uint dev_count;

void imcfreq_eard_load(topology_t *tp, imcfreq_ops_t *ops, int eard)
{
	uint imc_pack[2];

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
	if (state_fail(eards_imcfreq_data_count(imc_pack))) {
		return;
	}
	eard_api = imc_pack[0];
	dev_count = imc_pack[1];
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
	if (dev_count_in == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	*dev_count_in = dev_count;
	return EAR_SUCCESS;
}

state_t imcfreq_eard_read(ctx_t *c, imcfreq_t *i)
{
	if (i == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	memset((void *) i, 0, dev_count * sizeof(imcfreq_t));
	return eards_imcfreq_read(i, dev_count * sizeof(imcfreq_t));
}
