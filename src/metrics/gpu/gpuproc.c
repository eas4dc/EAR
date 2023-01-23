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

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <metrics/gpu/gpuproc.h>
#include <metrics/gpu/archs/cupti.h>

#include <common/system/monitor.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static gpuproc_ops_t ops;
static uint devs_count;
static uint ok_load;

void gpuproc_load(int eard)
{
    debug("gpuproc_load enters");
    while (pthread_mutex_trylock(&lock));
    if (ok_load) {
        goto unlock_load;
    }
    gpu_cupti_load(&ops);
    ok_load = 1;
unlock_load:
    pthread_mutex_unlock(&lock);
    debug("gpuproc_load exits");
}

void gpuproc_get_api(uint *api)
{
    *api = API_DUMMY;
    if (ops.get_api != NULL) {
        ops.get_api(api);
    }
}

state_t gpuproc_init(ctx_t *c)
{
    state_t s;
    if ((ops.init != NULL) && state_fail(s = ops.init(c))) {
        return s;
    }
    return gpuproc_count_devices(c, &devs_count);
}

state_t gpuproc_dispose(ctx_t *c)
{
    preturn(ops.dispose, c);
}

state_t gpuproc_count_devices(ctx_t *c, uint *devs_count)
{
    *devs_count = 1;
    if (ops.count_devices != NULL) {
        return ops.count_devices(c, devs_count);
    }
    return EAR_SUCCESS;
}

state_t gpuproc_read(ctx_t *c, gpuproc_t *data)
{
    gpuproc_data_null(data);
    if (ops.read != NULL) {
        return ops.read(c, data);
    }
    return EAR_SUCCESS;
}

state_t gpuproc_read_diff(ctx_t *c, gpuproc_t *data2, gpuproc_t *data1, gpuproc_t *dataD)
{
	state_t s;
	if (state_fail(s = gpuproc_read(c, data2))) {
		return s;
	}
	return gpuproc_data_diff(data2, data1, dataD);
}

state_t gpuproc_read_copy(ctx_t *c, gpuproc_t *data2, gpuproc_t *data1, gpuproc_t *dataD)
{
	state_t s;
	if (state_fail(s = gpuproc_read_diff(c, data2, data1, dataD))) {
		return s;
	}
	return gpuproc_data_copy(data1, data2);
}

state_t gpuproc_enable(ctx_t *c)
{
    return ops.enable(c);
}

state_t gpuproc_disable(ctx_t *c)
{
    return ops.disable(c);
}

static void static_data_diff(gpuproc_t *data2, gpuproc_t *data1, gpuproc_t *dataD, int i)
{
	gpuproc_t *d3 = &dataD[i];
	gpuproc_t *d2 = &data2[i];
	gpuproc_t *d1 = &data1[i];
	//ullong time_i;
	//double time_f;

	// Computing time
	//time_i = timestamp_diff(&d2->time, &d1->time, TIME_USECS);
	//time_f = ((double) time_i) / 1000000.0;
	// Averages
	if ((d3->samples = d2->samples - d1->samples) == 0) {
		memset(d3, 0, sizeof(gpuproc_t));
		return;
	}
	// No overflow control is required (64-bits are enough)
	d3->flops_sp = (d2->flops_sp - d1->flops_sp);
	d3->flops_dp = (d2->flops_dp - d1->flops_dp);
	d3->insts    = (d2->insts    - d1->insts);
	d3->cycles   = (d2->cycles   - d1->cycles);

    if (d3->insts > 0) {
	    d3->cpi  = ((double) d3->cycles) / ((double) d3->insts);
    }
	#if 0
	debug("%d flops_sp, %llu = %llu - %llu", i, d3->flops_sp, d2->flops_sp, d1->flops_sp);
	debug("%d flops_dp, %llu = %llu - %llu", i, d3->flops_dp, d2->flops_dp, d1->flops_dp);
    debug("%d cycles  , %llu = %llu - %llu", i, d3->cycles,   d2->cycles,   d1->cycles);
    debug("%d insts   , %llu = %llu - %llu", i, d3->insts,    d2->insts,    d1->insts);
    debug("%d cpi     , %0.2lf = %0.2lf - %0.2lf", i, d3->cpi, d2->cpi, d1->cpi);
	#endif
}

state_t gpuproc_data_diff(gpuproc_t *data2, gpuproc_t *data1, gpuproc_t *dataD)
{
	state_t s;
	int i;

	if (state_fail(s = gpuproc_data_null(dataD))) {
		return s;
	}
	for (i = 0; i < devs_count; i++) {
		static_data_diff(data2, data1, dataD, i);
	}
	#if 0
	gpuproc_data_print(data_diff, debug_channel);
	#endif
	return EAR_SUCCESS;
}

state_t gpuproc_data_alloc(gpuproc_t **data)
{
	if (data == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	*data = calloc(devs_count, sizeof(gpuproc_t));
	if (*data == NULL) {
		return_msg(EAR_SYSCALL_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t gpuproc_data_free(gpuproc_t **data)
{
	if (data != NULL) {
		free(*data);
	}
	return EAR_SUCCESS;
}

state_t gpuproc_data_null(gpuproc_t *data)
{
	if (data == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	memset(data, 0, devs_count * sizeof(gpuproc_t));
	return EAR_SUCCESS;
}

state_t gpuproc_data_copy(gpuproc_t *data_dst, gpuproc_t *data_src)
{
	if (data_dst == NULL || data_src == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	memcpy(data_dst, data_src, devs_count * sizeof(gpuproc_t));
	return EAR_SUCCESS;
}

void gpuproc_data_print(gpuproc_t *data, int fd)
{
    char buffer[2048];
    gpuproc_data_tostr(data, buffer, 2048);
    dprintf(fd, "%s", buffer);
}

void gpuproc_data_tostr(gpuproc_t *data, char *buffer, int length)
{
	int accuml;
	size_t s;
	int i;

	for (i = accuml = 0; i < devs_count && length > 0; ++i) {
		s = snprintf(&buffer[accuml], length,
			 "gpu%u: FLOPs %llu/%llu, instructions %llu, cycles %llu, CPI %0.2lf\n", i,
			 data[i].flops_sp, data[i].flops_dp, data[i].insts, data[i].cycles, data[i].cpi);
		length = length - s;
		accuml = accuml + s;
	}
}
