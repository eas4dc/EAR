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
#include <common/output/debug.h>
#include <metrics/temperature/temperature.h>
#include <metrics/temperature/archs/amd17.h>
#include <metrics/temperature/archs/intel63.h>
#include <metrics/temperature/archs/dummy.h>

static struct temp_ops
{
	state_t (*init)          (ctx_t *c);
	state_t (*dispose)       (ctx_t *c);
	state_t (*count_devices) (ctx_t *c, uint *count);
	state_t (*read)          (ctx_t *c, llong *temp, llong *average);
} ops;

state_t temp_load(topology_t *tp)
{
	if (ops.init != NULL) {
		return EAR_SUCCESS;
	}
	if (state_ok(temp_intel63_status(tp))) {
		ops.init          = temp_intel63_init;
		ops.dispose       = temp_intel63_dispose;
		ops.read          = temp_intel63_read;
		ops.count_devices = temp_intel63_count_devices;
		debug("selected intel63 temperature");
	} else if (state_ok(temp_amd17_status(tp))) {
		ops.init          = temp_amd17_init;
		ops.dispose       = temp_amd17_dispose;
		ops.read          = temp_amd17_read;
		ops.count_devices = temp_amd17_count_devices;
		debug("selected amd17 temperature");
	} else if (state_ok(temp_dummy_status(tp))) {
		ops.init          = temp_dummy_init;
		ops.dispose       = temp_dummy_dispose;
		ops.read          = temp_dummy_read;
		ops.count_devices = temp_dummy_count_devices;
		debug("selected dummy temperature");
	}
	if (ops.init == NULL) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	return EAR_SUCCESS;
}

state_t temp_init(ctx_t *c)
{
	preturn(ops.init, c);
}

state_t temp_dispose(ctx_t *c)
{
	preturn(ops.dispose, c);
}

state_t temp_count_devices(ctx_t *c, uint *count)
{
	preturn(ops.count_devices, c, count);
}

// Data
state_t temp_data_alloc(ctx_t *c, llong **temp, uint *temp_count)
{
	uint socket_count;
	state_t s;
	if (temp == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (xtate_fail(s, temp_count_devices(c, &socket_count))) {
		return s;
	}
	if ((*temp = (llong *) calloc(socket_count, sizeof(llong))) == NULL) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	if (temp_count != NULL) {
		*temp_count = socket_count;
	}
	return EAR_SUCCESS;
}

state_t temp_data_copy(ctx_t *c, llong *temp2, llong *temp1)
{
	uint socket_count;
	state_t s;
	if (temp2 == NULL || temp1 == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	if (xtate_fail(s, temp_count_devices(c, &socket_count))) {
		return s;
	}
	if (memcpy((void *) temp2, (const void *) temp1, sizeof(llong)*socket_count) != temp2) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t temp_data_free(ctx_t *c, llong **temp)
{
	if (temp == NULL || *temp == NULL) {
		return_msg(EAR_BAD_ARGUMENT, Generr.input_null);
	}
	free(*temp);
	*temp = NULL;
	return EAR_SUCCESS;
}

// Getters
state_t temp_read(ctx_t *c, llong *temp, llong *average)
{
	preturn(ops.read, c, temp, average);
}
