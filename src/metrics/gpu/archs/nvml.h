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

#ifndef METRICS_GPU_NVML
#define METRICS_GPU_NVML

#include <metrics/gpu/gpu.h>

state_t nvml_status();

state_t nvml_init(ctx_t *c);

state_t nvml_init_unprivileged(ctx_t *c);

state_t nvml_dispose(ctx_t *c);

state_t nvml_count(ctx_t *c, uint *devs_count);

state_t nvml_get_info(ctx_t *c, const gpu_info_t **info, uint *devs_count);

state_t nvml_pool(void *c);

state_t nvml_read(ctx_t *c, gpu_t *data);

state_t nvml_read_copy(ctx_t *c, gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

/* Reads the data directly from the GPU API (not preprocessed data). */
state_t nvml_read_raw(ctx_t *c, gpu_t *data);

state_t nvml_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

/* Computes the average/accum. between all the devices (data_avg length is 1). */
state_t nvml_data_merge(gpu_t *data_diff, gpu_t *data_merge);

state_t nvml_data_alloc(gpu_t **data);

state_t nvml_data_free(gpu_t **data);

state_t nvml_data_null(gpu_t *data);

state_t nvml_data_copy(gpu_t *data_dst, gpu_t *data_src);

state_t nvml_data_print(gpu_t *data, int fd);

state_t nvml_data_tostr(gpu_t *data, char *buffer, int length);

#endif //METRICS_GPU_NVML
