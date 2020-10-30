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
#include <common/output/debug.h>
#include <metrics/gpu/gpu.h>
#include <metrics/gpu/gpu/nvml.h>
#include <metrics/gpu/gpu/dummy.h>

static gpu_ops_t ops;
static uint loaded;
static uint model;

state_t gpu_load(gpu_ops_t **_ops, uint model_force, uint *model_used)
{
	if (loaded != 0) {
		return EAR_SUCCESS;
	}

	if (model_force == MODEL_NVML || state_ok(nvml_status()))
	{
		debug("loaded NVML");
		ops.init		= nvml_init;
		ops.dispose		= nvml_dispose;
		ops.read		= nvml_read;
		ops.read_copy	= nvml_read_copy;
		ops.count		= nvml_count;
		ops.data_init   = nvml_data_init;
		ops.data_diff	= nvml_data_diff;
		ops.data_alloc	= nvml_data_alloc;
		ops.data_free	= nvml_data_free;
		ops.data_null	= nvml_data_null;
		ops.data_diff	= nvml_data_diff;
		ops.data_copy	= nvml_data_copy;
		ops.data_print	= nvml_data_print;
		ops.data_tostr	= nvml_data_tostr;
		model			= MODEL_NVML;
		loaded			= 1;
	} else {
		debug("loaded DUMMY");
		ops.init		= gpu_dummy_init;
		ops.dispose		= gpu_dummy_dispose;
		ops.read		= gpu_dummy_read;
		ops.read_copy	= gpu_dummy_read_copy;
		ops.count		= gpu_dummy_count;
		ops.data_init   = gpu_dummy_data_init;
		ops.data_diff	= gpu_dummy_data_diff;
		ops.data_alloc	= gpu_dummy_data_alloc;
		ops.data_free	= gpu_dummy_data_free;
		ops.data_null	= gpu_dummy_data_null;
		ops.data_diff	= gpu_dummy_data_diff;
		ops.data_copy	= gpu_dummy_data_copy;
		ops.data_print	= gpu_dummy_data_print;
		ops.data_tostr	= gpu_dummy_data_tostr;
		model			= MODEL_UNDEFINED;
		loaded			= 1;
	}

	if (model_used != NULL) {
		*model_used = model;
	}
	if (_ops != NULL) {
		*_ops = &ops;
	}

	return EAR_SUCCESS;
}

state_t gpu_init(ctx_t *c)
{
	preturn (ops.init, c);
}

state_t gpu_dispose(ctx_t *c)
{
	preturn (ops.dispose, c);
}

state_t gpu_read(ctx_t *c, gpu_t *data)
{
	preturn (ops.read, c, data);
}

state_t gpu_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	preturn (ops.read_copy, c, data2, data1, data_diff);
}

state_t gpu_count(ctx_t *c, uint *dev_count)
{
	preturn (ops.count, c, dev_count);
}

state_t gpu_data_init(uint dev_count)
{
	preturn (ops.data_init, dev_count);
}

state_t gpu_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	preturn (ops.data_diff, data2, data1, data_diff);
}

state_t gpu_data_alloc(gpu_t **data)
{
	preturn (ops.data_alloc, data);
}

state_t gpu_data_free(gpu_t **data)
{
	preturn (ops.data_free, data);
}

state_t gpu_data_null(gpu_t *data)
{
	preturn (ops.data_null, data);
}

state_t gpu_data_copy(gpu_t *data_dst, gpu_t *data_src)
{
	preturn (ops.data_copy, data_dst, data_src);
}

state_t gpu_data_print(gpu_t *data, int fd)
{
	preturn (ops.data_print, data, fd);
}

state_t gpu_data_tostr(gpu_t *data, char *buffer, int length)
{
	preturn (ops.data_tostr, data, buffer, length);
}
