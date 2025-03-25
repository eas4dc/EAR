/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdlib.h>
#include <pthread.h>

// #define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <metrics/gpu/gpu.h>
#include <metrics/gpu/archs/eard.h>
#include <metrics/gpu/archs/nvml.h>
#include <metrics/gpu/archs/dummy.h>
#include <metrics/gpu/archs/oneapi.h>
#include <metrics/gpu/archs/pvc_power_hwmon.h>
#include <metrics/gpu/archs/rsmi.h>

#ifndef USE_PVC_HWMON
#warning "USE_PVC_HWMON not defined! Defining it to 0..."
#define USE_PVC_HWMON 0
#endif

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static gpu_ops_t       ops;
static uint            devs_count;
static uint            ok_load;

void gpu_load(int force_api)
{
    while (pthread_mutex_trylock(&lock));
    if (ok_load) {
        goto unlock_load;
    }
    if (API_IS(force_api, API_DUMMY)) {
        goto dummy;
    }
    gpu_nvml_load(&ops, force_api);
    gpu_rsmi_load(&ops, force_api);
		if (USE_PVC_HWMON) {
			if (state_fail(gpu_pvc_hwmon_load(&ops, force_api))) {
				debug("GPU PVC HWMON Load error: %s", state_msg);
			}
		} else {
			gpu_oneapi_load(&ops, force_api);
		}
    gpu_eard_load(&ops, API_IS(force_api, API_EARD));
dummy:
    gpu_dummy_load(&ops);
    ok_load = 1;
unlock_load:
    pthread_mutex_unlock(&lock);
}

void gpu_get_api(uint *api)
{
    apinfo_t info;
    gpu_get_info(&info);
    *api = info.api;
}

void gpu_get_info(apinfo_t *info)
{
    info->layer       = "GPU";
    info->api         = API_NONE;
    info->devs_count  = 0;
    info->scope       = SCOPE_NODE;
    info->granularity = GRANULARITY_PERIPHERAL;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

void gpu_get_devices(gpu_devs_t **devs, uint *devs_count)
{
    if (ops.get_devices != NULL) {
        ops.get_devices(devs, devs_count);
    }
}

state_t gpu_init(ctx_t *c)
{
    state_t s = EAR_ERROR;
    while (pthread_mutex_trylock(&lock));
    if (ops.init != NULL) {
        if (state_fail(s = ops.init(c))) {
            goto unlock_init;
        }
    }
    // Number of devices are used in data functions
    gpu_get_devices(NULL, &devs_count);
unlock_init:
    pthread_mutex_unlock(&lock);
    return s;
}

state_t gpu_dispose(ctx_t *c)
{
    state_t s = EAR_SUCCESS;
    while (pthread_mutex_trylock(&lock));
    if (ops.dispose != NULL) {
        s = ops.dispose(c);
    }
    pthread_mutex_unlock(&lock);
    return s;
}

void gpu_set_monitoring_mode(int mode)
{
    if (ops.set_monitoring_mode != NULL) {
        ops.set_monitoring_mode(mode);
    }
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
	gpu_data_diff(data2, data1, data_diff);
    return s;
}

state_t gpu_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	state_t s;
	if (state_fail(s = gpu_read_diff(c, data2, data1, data_diff))) {
		return s;
	}
	gpu_data_copy(data1, data2);
    return s;
}

static void static_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff, int i)
{
	gpu_t *d3 = &data_diff[i];
	gpu_t *d2 = &data2[i];
	gpu_t *d1 = &data1[i];

	if (d2->correct != 1 || d1->correct != 1) {
		return;
	}
	// Averages
	if ((d3->samples = d2->samples - d1->samples) == 0) {
		memset(d3, 0, sizeof(gpu_t));
		return;
	}

	// Computing time
	double time_f = timestamp_diff(&d2->time, &d1->time, TIME_SECS);
	debug("static_data_diff accumulated time diff %lf (d2 %lu d1 %lu)", time_f, d2->time.tv_sec, d1->time.tv_sec);

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
	if (d3->power_w == 0){
		d3->power_w = d3->energy_j / time_f;
		debug("WARNING: Computing power using energy : %lf power = %lf energy / %lf secs", d3->power_w, d3->energy_j, time_f);
	}


	#if 1
	debug("%d freq gpu (KHz), %lu = %lu - %lu", i, d3->freq_gpu, d2->freq_gpu, d1->freq_gpu);
    debug("%d freq mem (KHz), %lu = %lu - %lu", i, d3->freq_mem, d2->freq_mem, d1->freq_mem);
    debug("%d power    (W)  , %0.2lf = %0.2lf - %0.2lf", i, d3->power_w, d2->power_w, d1->power_w);
    debug("%d energy   (J)  , %0.2lf = %0.2lf - %0.2lf", i, d3->energy_j, d2->energy_j, d1->energy_j);
	#endif
}

void gpu_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	int i;
	gpu_data_null(data_diff);
	for (i = 0; i < devs_count; i++) {
		static_data_diff(data2, data1, data_diff, i);
	}
}

void gpu_data_merge(gpu_t *data_diff, gpu_t *data_merge)
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
}

void gpu_data_alloc(gpu_t **data)
{
	*data = calloc(devs_count, sizeof(gpu_t));
}

void gpu_data_free(gpu_t **data)
{
    free(*data);
    *data = NULL;
}

void gpu_data_null(gpu_t *data)
{
	memset(data, 0, devs_count * sizeof(gpu_t));
}

void gpu_data_copy(gpu_t *data_dst, gpu_t *data_src)
{
	memcpy(data_dst, data_src, devs_count * sizeof(gpu_t));
}

void gpu_data_print(gpu_t *data, int fd)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
		dprintf(fd, "gpu%u: %3.0lf J, %3.0lf W, %4lu MHz, %4lu MHz, %2lu-%2lu usg, %2lu-%lu t, %lu\n", i,
			data[i].energy_j, data[i].power_w,
			data[i].freq_gpu / 1000LU, data[i].freq_mem / 1000LU,
			data[i].util_gpu, data[i].util_mem,
			data[i].temp_gpu, data[i].temp_mem,
			data[i].samples);
	}
}

char *gpu_data_tostr(gpu_t *data, char *buffer, int length)
{
    char *state[2] = { "er", "ok" };
	int accuml;
	size_t s;
	int i;

	for (i = accuml = 0; i < devs_count && length > 0; ++i) {
		s = snprintf(&buffer[accuml], length,
			 "gpu%u: %3.0lf J, %3.0lf W, %7lu KHz, %7lu KHz, %2lu-%2lu usg, %2lu-%lu t, %lu (%s)\n", i,
			 data[i].energy_j, data[i].power_w,
			 data[i].freq_gpu, data[i].freq_mem,
			 data[i].util_gpu, data[i].util_mem,
			 data[i].temp_gpu, data[i].temp_mem,
             data[i].samples, state[data[i].correct]);
		length = length - s;
		accuml = accuml + s;
	}
    return buffer;
}

int gpu_is_supported()
{
    return gpu_eard_is_supported();
}
