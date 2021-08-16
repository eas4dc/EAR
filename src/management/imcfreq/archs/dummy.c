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

#include <stdlib.h>
#include <management/imcfreq/archs/dummy.h>

uint sockets_count;

state_t mgt_imcfreq_dummy_load(topology_t *tp, mgt_imcfreq_ops_t *ops)
{
	replace_ops(ops->init,                 mgt_imcfreq_dummy_init);
	replace_ops(ops->dispose,              mgt_imcfreq_dummy_dispose);
	replace_ops(ops->count_devices,        mgt_imcfreq_dummy_count_devices);
	replace_ops(ops->get_available_list,   mgt_imcfreq_dummy_get_available_list);
	replace_ops(ops->get_current_list,     mgt_imcfreq_dummy_get_current_list);
	replace_ops(ops->set_current_list,     mgt_imcfreq_dummy_set_current_list);
	replace_ops(ops->set_current,          mgt_imcfreq_dummy_set_current);
	replace_ops(ops->set_auto,             mgt_imcfreq_dummy_set_auto);
	replace_ops(ops->get_current_ranged_list, mgt_imcfreq_dummy_get_current_ranged_list);
	replace_ops(ops->set_current_ranged_list, mgt_imcfreq_dummy_set_current_ranged_list);
	replace_ops(ops->data_alloc,           mgt_imcfreq_dummy_data_alloc);
	sockets_count = tp->socket_count;
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_dummy_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_imcfreq_dummy_count_devices(ctx_t *c, uint *devs_count)
{
	*devs_count = sockets_count;
    return EAR_SUCCESS;
}

/** */
state_t mgt_imcfreq_dummy_get_available_list(ctx_t *c, const pstate_t **pstate_list, uint *count)
{
	static pstate_t ps[1];
	ps[0].khz = 1000000LLU;
	ps[0].idx = 0;
	*pstate_list = ps;
	if (count != NULL) {
		*count = 1;
	}
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_dummy_get_current_list(ctx_t *c, pstate_t *pstate_list)
{
	int i;
	for (i = 0; i < sockets_count; ++i) {
		pstate_list[i].idx = 0;
		pstate_list[i].khz = 1000000LLU;
	}
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_dummy_set_current_list(ctx_t *c, uint *index_list)
{
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_dummy_set_current(ctx_t *c, uint pstate_index, int socket)
{
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_dummy_set_auto(ctx_t *c)
{
	return EAR_SUCCESS;
}

/** */
state_t mgt_imcfreq_dummy_get_current_ranged_list(ctx_t *c, pstate_t *ps_min_list, pstate_t *ps_max_list)
{
	return EAR_SUCCESS;
}

state_t mgt_imcfreq_dummy_set_current_ranged_list(ctx_t *c, uint *id_min_list, uint *id_max_list)
{
	return EAR_SUCCESS;
}

/** */
state_t mgt_imcfreq_dummy_data_alloc(pstate_t **pstate_list, uint **index_list)
{
	if (pstate_list != NULL) {
		*pstate_list = calloc(sockets_count, sizeof(pstate_t));
	}
	if (index_list != NULL) {
		*index_list = calloc(sockets_count, sizeof(uint));
	}
	return EAR_SUCCESS;
}
