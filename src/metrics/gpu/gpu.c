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

#include <stdlib.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <metrics/gpu/gpu.h>
#include <metrics/gpu/archs/eard.h>
#include <metrics/gpu/archs/nvml.h>
#include <metrics/gpu/archs/dummy.h>
#include <metrics/gpu/archs/cupti.h>
#include <metrics/gpu/archs/oneapi.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static gpu_ops_t ops;
static uint devs_count;
static uint ok_load;

void gpu_load(int eard)
{
    while (pthread_mutex_trylock(&lock));
    if (ok_load) {
        goto unlock_load;
    }
    // NVML and ONEAPI are fully loaded.
    // If not, EARD is fully loaded.
    gpu_nvml_load(&ops, eard);
    gpu_oneapi_load(&ops);
    gpu_eard_load(&ops, eard);
    gpu_dummy_load(&ops);
    ok_load = 1;
unlock_load:
    pthread_mutex_unlock(&lock);
}

void gpu_get_api(uint *api)
{
    *api = API_NONE;
    if (ops.get_api != NULL) {
        return ops.get_api(api);
    }
}

state_t gpu_init(ctx_t *c)
{
    if (ops.init != NULL) {
        ops.init(c);
    }
    return gpu_count_devices(c, &devs_count);
}

state_t gpu_dispose(ctx_t *c)
{
    preturn(ops.dispose, c);
}

state_t gpu_get_devices(ctx_t *c, gpu_devs_t **devs, uint *devs_count)
{
    preturn (ops.get_devices, c, devs, devs_count);
}

state_t gpu_count_devices(ctx_t *c, uint *devs_count)
{
	preturn(ops.count_devices, c, devs_count);
}

state_t gpu_read(ctx_t *c, gpu_t *data)
{
    preturn(ops.read, c, data);
}

state_t gpu_read_raw(ctx_t *c, gpu_t *data)
{
	preturn(ops.read_raw, c, data);
}

state_t gpu_read_diff(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	state_t s;
	if (state_fail(s = gpu_read(c, data2))) {
		return s;
	}
	return gpu_data_diff(data2, data1, data_diff);
}

state_t gpu_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	state_t s;
	if (state_fail(s = gpu_read_diff(c, data2, data1, data_diff))) {
		return s;
	}
	return gpu_data_copy(data1, data2);
}

static void static_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff, int i)
{
	gpu_t *d3 = &data_diff[i];
	gpu_t *d2 = &data2[i];
	gpu_t *d1 = &data1[i];
	//ullong time_i;
	//double time_f;

	if (d2->correct != 1 || d1->correct != 1) {
		return;
	}
	// Computing time
	//time_i = timestamp_diff(&d2->time, &d1->time, TIME_USECS);
	//time_f = ((double) time_i) / 1000000.0;
	// Averages
	if ((d3->samples = d2->samples - d1->samples) == 0) {
		memset(d3, 0, sizeof(gpu_t));
		return;
	}
	// No overflow control is required (64-bits are enough)
	d3->freq_gpu = (d2->freq_gpu - d1->freq_gpu) / d3->samples;
	d3->freq_mem = (d2->freq_mem - d1->freq_mem) / d3->samples;
	d3->util_gpu = (d2->util_gpu - d1->util_gpu) / d3->samples;
	d3->util_mem = (d2->util_mem - d1->util_mem) / d3->samples;
	d3->temp_gpu = (d2->temp_gpu - d1->temp_gpu) / d3->samples;
	d3->temp_mem = (d2->temp_mem - d1->temp_mem) / d3->samples;
	d3->power_w  = (d2->power_w  - d1->power_w ) / d3->samples;
	d3->energy_j = (d2->energy_j - d1->energy_j);
	d3->working  = (d2->working);
	d3->correct  = 1;

	#if 0
	debug("%d freq gpu (KHz), %lu = %lu - %lu", i, d3->freq_gpu, d2->freq_gpu, d1->freq_gpu);
    debug("%d freq mem (KHz), %lu = %lu - %lu", i, d3->freq_mem, d2->freq_mem, d1->freq_mem);
    debug("%d power    (W)  , %0.2lf = %0.2lf - %0.2lf", i, d3->power_w, d2->power_w, d1->power_w);
    debug("%d energy   (J)  , %0.2lf = %0.2lf - %0.2lf", i, d3->energy_j, d2->energy_j, d1->energy_j);
	#endif
}

state_t gpu_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	state_t s;
	int i;

	if (state_fail(s = gpu_data_null(data_diff))) {
		return s;
	}
	for (i = 0; i < devs_count; i++) {
		static_data_diff(data2, data1, data_diff, i);
	}
	#if 0
	gpu_data_print(data_diff, debug_channel);
	#endif
	return EAR_SUCCESS;
}

state_t gpu_data_merge(gpu_t *data_diff, gpu_t *data_merge)
{
	int i;

	// Cleaning (do not use null, because is just 1 sample)
	memset((void *) data_merge, 0, sizeof(gpu_t));
	// Accumulators: power and energy
	for (i = 0; i < devs_count; ++i) {
		data_merge->freq_mem += data_diff[i].freq_mem;
		data_merge->freq_gpu += data_diff[i].freq_gpu;
		data_merge->util_mem += data_diff[i].util_mem;
		data_merge->util_gpu += data_diff[i].util_gpu;
		data_merge->temp_gpu += data_diff[i].temp_gpu;
		data_merge->temp_mem += data_diff[i].temp_mem;
		data_merge->energy_j += data_diff[i].energy_j;
		data_merge->power_w  += data_diff[i].power_w;
	}
	// Static
	data_merge->time    = data_diff[0].time;
	data_merge->samples = data_diff[0].samples;
	// Averages
	data_merge->freq_mem /= devs_count;
	data_merge->freq_gpu /= devs_count;
	data_merge->util_mem /= devs_count;
	data_merge->util_gpu /= devs_count;
	data_merge->temp_gpu /= devs_count;
	data_merge->temp_mem /= devs_count;

	return EAR_SUCCESS;
}

state_t gpu_data_alloc(gpu_t **data)
{
	if (data == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	*data = calloc(devs_count, sizeof(gpu_t));
	if (*data == NULL) {
		return_msg(EAR_SYSCALL_ERROR, strerror(errno));
	}
	return EAR_SUCCESS;
}

state_t gpu_data_free(gpu_t **data)
{
	if (data != NULL) {
		free(*data);
	}
	return EAR_SUCCESS;
}

state_t gpu_data_null(gpu_t *data)
{
	if (data == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	memset(data, 0, devs_count * sizeof(gpu_t));
	return EAR_SUCCESS;
}

state_t gpu_data_copy(gpu_t *data_dst, gpu_t *data_src)
{
	if (data_dst == NULL || data_src == NULL) {
		return_msg(EAR_ERROR, Generr.input_null);
	}
	memcpy(data_dst, data_src, devs_count * sizeof(gpu_t));
	return EAR_SUCCESS;
}

void gpu_data_print(gpu_t *data, int fd)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
		dprintf(fd, "gpu%u: %0.2lfJ, %0.2lfW, %luMHz, %luMHz, %lu%%, %lu%%, %luº, %luº, %u, %lu\n", i,
			data[i].energy_j, data[i].power_w,
			data[i].freq_gpu, data[i].freq_mem,
			data[i].util_gpu, data[i].util_mem,
			data[i].temp_gpu, data[i].temp_mem,
			data[i].working , data[i].samples);
	}
}

void gpu_data_tostr(gpu_t *data, char *buffer, int length)
{
	int accuml;
	size_t s;
	int i;

	for (i = accuml = 0; i < devs_count && length > 0; ++i) {
		s = snprintf(&buffer[accuml], length,
			 "gpu%u: %0.2lfJ, %0.2lfW, %luKHz, %luMHz, %lu, %lu, %lu, %lu, %u, %lu correct=%u\n", i,
			 data[i].energy_j, data[i].power_w,
			 data[i].freq_gpu, data[i].freq_mem,
			 data[i].util_gpu, data[i].util_mem,
			 data[i].temp_gpu, data[i].temp_mem,
			 data[i].working , data[i].samples,
			 data[i].correct);
		length = length - s;
		accuml = accuml + s;
	}
}

int gpu_is_supported()
{
    return gpu_eard_is_supported();
}
