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

#include <metrics/bandwidth/cpu/dummy.h>

state_t bwidth_dummy_init(ctx_t *c, topology_t *tp)
{
	return EAR_SUCCESS;
}

state_t bwidth_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t bwidth_dummy_count(ctx_t *c, uint *count)
{
	*count = 1;
	return EAR_SUCCESS;
}

state_t bwidth_dummy_start(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t bwidth_dummy_reset(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t bwidth_dummy_stop(ctx_t *c, ullong *cas)
{
	return bwidth_dummy_read(c, cas);
}

state_t bwidth_dummy_read(ctx_t *c, ullong *cas)
{
	if (cas != NULL) {
		*cas = 1;
	}
	return EAR_SUCCESS;
}
