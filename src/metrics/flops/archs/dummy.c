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
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1
#include <string.h>
#include <common/output/debug.h>
#include <metrics/flops/archs/dummy.h>

void flops_dummy_load(topology_t *tp, flops_ops_t *ops)
{
    apis_put(ops->get_api,         flops_dummy_get_api);
    apis_put(ops->get_granularity, flops_dummy_get_granularity);
    apis_put(ops->get_weights,     flops_dummy_get_weights);
    apis_put(ops->init,            flops_dummy_init);
    apis_put(ops->dispose,         flops_dummy_dispose);
    apis_put(ops->read,            flops_dummy_read);
}

void flops_dummy_get_api(uint *api)
{
    *api = API_DUMMY;
}

void flops_dummy_get_granularity(uint *granularity)
{
    *granularity = GRANULARITY_DUMMY;
}

void flops_dummy_get_weights(ullong **weights_in)
{
    static ullong weights[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    *weights_in = weights;
}

state_t flops_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t flops_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t flops_dummy_read(ctx_t *c, flops_t *fl)
{
	memset(fl, 0, sizeof(flops_t));
	return EAR_SUCCESS;
}
