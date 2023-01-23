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

#include <errno.h>
#include <stdlib.h>
#include <metrics/energy_cpu/archs/dummy.h>

static uint socket_count;

state_t energy_cpu_dummy_load(topology_t *topo)
{
	if (socket_count == 0) {
		if (topo->socket_count > 0) {
			socket_count = topo->socket_count;
		} else {
			socket_count = 1;
		}
	}
	return EAR_SUCCESS;
}

state_t energy_cpu_dummy_init(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t energy_cpu_dummy_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

// Data
state_t energy_cpu_dummy_count_devices(ctx_t *c, uint *count)
{
	if (count != NULL) {
		*count = socket_count;
	}
	return EAR_SUCCESS;
}

// Getters
state_t energy_cpu_dummy_read(ctx_t *c, ullong *values)
{
	if (values != NULL) {
		if (memset((void *) values, 0, NUM_PACKS*sizeof(ullong)*socket_count) != values) {
			return_msg(EAR_ERROR, strerror(errno));
		}
	}
	return EAR_SUCCESS;
}

