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

#include <library/metrics/gpu.h>

static gpu_ops_t *ops;
static uint dev_count;

state_t gpu_lib_load(settings_conf_t *settings)
{
	// HERE: Get the GPU model identificator, look
	// 'metrics/gpu/gpu.c' for the model list.

	// Replace 0 by GPU model
	return gpu_load(&ops, 0, NULL);
}

state_t gpu_lib_init(ctx_t *c)
{
	// HERE: Ask Daemon for dev_count

	preturn (ops->data_init, dev_count);
}

state_t gpu_lib_dispose(ctx_t *c)
{
	return EAR_SUCCESS;
}

state_t gpu_lib_read(ctx_t *c, gpu_t *data)
{
	// HERE: Ask daemon for GPU metrics
	
	return EAR_SUCCESS;
}

state_t gpu_lib_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	state_t s;
	if (xtate_fail(s, gpu_lib_read(c, data2))) {
		return s;
	}
	if (xtate_fail(s, ops->data_diff(data2, data1, data_diff))) {
		return s;
	}
	return ops->data_copy(data1, data2);
}

state_t gpu_lib_count(ctx_t *c, uint *_dev_count)
{
	*_dev_count = dev_count;
	return EAR_SUCCESS;
}

state_t gpu_lib_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff)
{
	preturn (ops->data_diff, data2, data1, data_diff);
}

state_t gpu_lib_data_alloc(gpu_t **data)
{
	preturn (ops->data_alloc, data);
}

state_t gpu_lib_data_free(gpu_t **data)
{
	preturn (ops->data_free, data);
}

state_t gpu_lib_data_null(gpu_t *data)
{
	preturn (ops->data_null, data);
}

state_t gpu_lib_data_copy(gpu_t *data_dst, gpu_t *data_src)
{
	preturn (ops->data_copy, data_dst, data_src);
}

state_t gpu_lib_data_print(gpu_t *data, int fd)
{
	preturn (ops->data_print, data, fd);
}

state_t gpu_lib_data_tostr(gpu_t *data, char *buffer, int length)
{
	preturn (ops->data_tostr, data, buffer, length);
}
