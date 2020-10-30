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

#ifndef METRICS_GPU_H
#define METRICS_GPU_H

#include <common/types.h>
#include <common/states.h>
#include <common/plugins.h>
#include <common/system/time.h>

#define MODEL_NVML			1
#define MODEL_UNDEFINED		0

typedef struct gpu_s
{
	timestamp_t time;
	ulong samples;
	ulong freq_gpu_mhz;
	ulong freq_mem_mhz;
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
	state_t (*init)		(ctx_t *c);
	state_t (*dispose)	(ctx_t *c);
	state_t (*read)		(ctx_t *c, gpu_t *data);
	state_t (*read_copy)	(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff);
	state_t (*count)	(ctx_t *c, uint *gpu_count);
	state_t (*data_init)	(uint dev_count);
	state_t (*data_diff)	(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);
	state_t (*data_alloc)	(gpu_t **data);
	state_t (*data_free)	(gpu_t **data);
	state_t (*data_null)	(gpu_t *data);
	state_t (*data_copy)	(gpu_t *data_dst, gpu_t *data_src);
	state_t (*data_print)	(gpu_t *data, int fd);
	state_t (*data_tostr)	(gpu_t *data, char *buffer, int length);
} gpu_ops_t;

state_t gpu_load(gpu_ops_t **ops, uint model_force, uint *model_used);

state_t gpu_init(ctx_t *c);

state_t gpu_dispose(ctx_t *c);

state_t gpu_count(ctx_t *c, uint *dev_count);

state_t gpu_read(ctx_t *c, gpu_t *data);

state_t gpu_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

state_t gpu_data_init(uint dev_count);

state_t gpu_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

state_t gpu_data_alloc(gpu_t **data);

state_t gpu_data_free(gpu_t **data);

state_t gpu_data_null(gpu_t *data);

state_t gpu_data_copy(gpu_t *data_dst, gpu_t *data_src);

state_t gpu_data_print(gpu_t *data, int fd);

state_t gpu_data_tostr(gpu_t *data, char *buffer, int length);

#endif
