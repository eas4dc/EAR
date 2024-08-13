/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_GPU_PROC_H
#define METRICS_GPU_PROC_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>
#include <metrics/common/apis.h>

// This is an API to monitorize the GPU devices of the process.

typedef struct gpuproc_s
{
    timestamp_t time;
    ullong samples;
    ullong flops_dp;
    ullong flops_sp;
    ullong insts;
    ullong cycles;
    double cpi;
} gpuproc_t;

typedef struct gpuproc_ops_s
{
    void    (*get_api)       (uint *api);
    state_t (*init)          (ctx_t *c);
    state_t (*dispose)       (ctx_t *c);
    state_t (*count_devices) (ctx_t *c, uint *devs_count);
    state_t (*read)          (ctx_t *c, gpuproc_t *data);
    state_t (*enable)        (ctx_t *c);
    state_t (*disable)       (ctx_t *c);
} gpuproc_ops_t;

// Discovers the low level API.
void gpuproc_load(int eard);

void gpuproc_get_api(uint *api);

// Initializes the context.
state_t gpuproc_init(ctx_t *c);

state_t gpuproc_dispose(ctx_t *c);

// Counts the number of GPUs (devices) in the node.
state_t gpuproc_count_devices(ctx_t *c, uint *dev_count);

// Reads the GPU device data and stores it in the gpuproc_t array data (1 per device).
state_t gpuproc_read(ctx_t *c, gpuproc_t *data);

// Performs a gpuproc_read() over data2, a gpuproc_data_diff() and copies data2 in data1.
state_t gpuproc_read_diff(ctx_t *c, gpuproc_t *data2, gpuproc_t *data1, gpuproc_t *data_diff);

// Performs a gpuproc_read() over data2, a gpuproc_data_diff() and copies data2 in data1.
state_t gpuproc_read_copy(ctx_t *c, gpuproc_t *data2, gpuproc_t *data1, gpuproc_t *data_diff);

// Enables or starts GPU metrics gathering (its enabled by default after the init)
state_t gpuproc_enable(ctx_t *c);

// Disables or stops GPU metrics gathering
state_t gpuproc_disable(ctx_t *c);

/** Helpers */
// Substracts the elements of the gpuproc_t array (data_diff = data2 - data1).
state_t gpuproc_data_diff(gpuproc_t *data2, gpuproc_t *data1, gpuproc_t *data_diff);

// Allocates an array of gpuproc_t (1 per device).
state_t gpuproc_data_alloc(gpuproc_t **data);

// Frees an array of gpuproc_t.
state_t gpuproc_data_free(gpuproc_t **data);

// Sets to 0 an array of gpuproc_t.
state_t gpuproc_data_null(gpuproc_t *data);

// Copies an array of gpuproc_t.
state_t gpuproc_data_copy(gpuproc_t *data_dst, gpuproc_t *data_src);

// Prints an array of gpuproc_t in the channel fd.
void gpuproc_data_print(gpuproc_t *data, int fd);

// Copies the printing string in a char buffer.
void gpuproc_data_tostr(gpuproc_t *data, char *buffer, int length);

#endif
