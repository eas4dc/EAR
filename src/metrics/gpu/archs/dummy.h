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

#ifndef METRICS_GPU_DUMMY
#define METRICS_GPU_DUMMY

#include <metrics/gpu/gpu.h>

state_t gpu_dummy_status();

state_t gpu_dummy_init(ctx_t *c);

state_t gpu_dummy_dispose(ctx_t *c);

state_t gpu_dummy_count(ctx_t *c, uint *dev_count);

state_t gpu_dummy_get_info(ctx_t *c, const gpu_info_t **info, uint *dev_count);

state_t gpu_dummy_read(ctx_t *c, gpu_t *data);

state_t gpu_dummy_read_raw(ctx_t *c, gpu_t *data);

state_t gpu_dummy_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

state_t gpu_dummy_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

state_t gpu_dummy_data_merge(gpu_t *data_diff, gpu_t *data_merge);

state_t gpu_dummy_data_alloc(gpu_t **data);

state_t gpu_dummy_data_free(gpu_t **data);

state_t gpu_dummy_data_null(gpu_t *data);

state_t gpu_dummy_data_copy(gpu_t *data_dst, gpu_t *data_src);

state_t gpu_dummy_data_print(gpu_t *data, int fd);

state_t gpu_dummy_data_tostr(gpu_t *data, char *buffer, int length);

#endif
