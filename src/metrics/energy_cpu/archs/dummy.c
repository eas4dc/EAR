/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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

