/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_GPU_H
#define METRICS_GPU_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <metrics/common/apis.h>
// #include <metrics/gpu/archs/dcgmi.h> // TODO: To be removed

typedef struct gpu_devs_s {
    ullong serial;
    uint index;
} gpu_devs_t;

typedef struct gpu_s
{
	timestamp_t time;
	ulong samples;
	ulong freq_gpu; // khz
	ulong freq_mem; // khz
	ulong util_gpu; // percent
	ulong util_mem; // percent
	ulong temp_gpu; // celsius
	ulong temp_mem; // celsius
	double energy_j;
	double power_w;
	uint working;
	uint correct;
} gpu_t;

typedef struct gpu_ops_s
{
    void    (*get_info)            (apinfo_t *info);
    void    (*get_devices)         (gpu_devs_t **devs, uint *devs_count);
    state_t (*init)                (ctx_t *c);
    state_t (*dispose)             (ctx_t *c);
    void    (*set_monitoring_mode) (int mode);
    state_t (*read)                (ctx_t *c, gpu_t *data);
    state_t (*read_raw)            (ctx_t *c, gpu_t *data);
} gpu_ops_t;

// Discovers the low level API.
void gpu_load(int force_api);

// Deprecated, switch to gpu_get_info
void gpu_get_api(uint *api);

// Returns all available static information (Work in progress)
void gpu_get_info(apinfo_t *info);

// Count devices by calling get_devices
#define gpu_count_devices(c, devs_count) \
    gpu_get_devices(NULL, devs_count)

// Information about devices
void gpu_get_devices(gpu_devs_t **devs, uint *devs_count);

// Initializes the context.
state_t gpu_init(ctx_t *c);

state_t gpu_dispose(ctx_t *c);

// You can use it to increase the monitoring rate.
void gpu_set_monitoring_mode(int mode);

// Reads the GPU device data and stores it in the gpu_t array data (1 per device).
state_t gpu_read(ctx_t *c, gpu_t *data);

state_t gpu_metrics_set();

state_t gpu_metrics_read();

// Reads the GPU device data directly from the hardware (not pooled).
state_t gpu_read_raw(ctx_t *c, gpu_t *data);

// Performs a gpu_read() over data2, a gpu_data_diff() and copies data2 in data1.
state_t gpu_read_diff(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

// Performs a gpu_read() over data2, a gpu_data_diff() and copies data2 in data1.
state_t gpu_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

/** Helpers */
// Substracts the elements of the gpu_t array (data_diff = data2 - data1).
void gpu_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

// Makes an average of all the elements of the data_diff array.
void gpu_data_merge(gpu_t *data_diff, gpu_t *data_merge);

// Allocates an array of gpu_t (1 per device).
void gpu_data_alloc(gpu_t **data);

// Frees an array of gpu_t.
void gpu_data_free(gpu_t **data);

// Sets to 0 an array of gpu_t.
void gpu_data_null(gpu_t *data);

// Copies an array of gpu_t.
void gpu_data_copy(gpu_t *data_dst, gpu_t *data_src);

// Prints an array of gpu_t in the channel fd.
void gpu_data_print(gpu_t *data, int fd);

// Copies the printing string in a char buffer.
char *gpu_data_tostr(gpu_t *data, char *buffer, int length);

int gpu_is_supported();

#endif
