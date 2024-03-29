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

#include <stdlib.h>
#include <common/output/debug.h>
#include <metrics/common/apis.h>
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

static uint api = API_NONE;
static uint devs_count;

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
        api = API_INTEL63;
	} else if (state_ok(temp_amd17_status(tp))) {
		ops.init          = temp_amd17_init;
		ops.dispose       = temp_amd17_dispose;
		ops.read          = temp_amd17_read;
		ops.count_devices = temp_amd17_count_devices;
		debug("selected amd17 temperature");
        api = API_AMD17;
	} else if (state_ok(temp_dummy_status(tp))) {
		ops.init          = temp_dummy_init;
		ops.dispose       = temp_dummy_dispose;
		ops.read          = temp_dummy_read;
		ops.count_devices = temp_dummy_count_devices;
		debug("selected dummy temperature");
        api = API_DUMMY;
	}
	if (ops.init == NULL) {
		return_msg(EAR_ERROR, Generr.api_incompatible);
	}
	return EAR_SUCCESS;
}

void temp_get_api(uint *api_in)
{
    *api_in = api;
}

state_t temp_init(ctx_t *c)
{
    state_t s = EAR_SUCCESS;
    if (state_fail(s = ops.init(c))) {
        return s;
    }
	if (state_fail(s = temp_count_devices(c, &devs_count))) {
		return s;
	}
    return s;
}

state_t temp_dispose(ctx_t *c)
{
	preturn(ops.dispose, c);
}

state_t temp_count_devices(ctx_t *c, uint *count)
{
	preturn(ops.count_devices, c, count);
}

// Getters
state_t temp_read(ctx_t *c, llong *temp, llong *average)
{
	preturn(ops.read, c, temp, average);
}

// Data
state_t temp_data_alloc(llong **temp)
{
	if ((*temp = (llong *) calloc(devs_count, sizeof(llong))) == NULL) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t temp_data_copy(llong *temp2, llong *temp1)
{
	if (memcpy((void *) temp2, (const void *) temp1, sizeof(llong)*devs_count) != temp2) {
		return_msg(EAR_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t temp_data_free(llong **temp)
{
	free(*temp);
	*temp = NULL;
	return EAR_SUCCESS;
}

void temp_data_print(llong *list, llong avrg, int fd)
{
    char buffer[1024];
    temp_data_tostr(list, avrg, buffer, 1024);
    dprintf(fd, "%s", buffer); 
}

char *temp_data_tostr(llong *list, llong avrg, char *buffer, int length)
{
    ullong a = 0;
    int i;

    for (i = 0; i < devs_count; ++i) {
        a += sprintf(&buffer[a], "%lld ", list[i]);
    }
    sprintf(&buffer[a], "!%lld\n", avrg);
    return buffer;
}