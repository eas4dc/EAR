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

#ifndef METRICS_GPU_ONEAPI_H
#define METRICS_GPU_ONEAPI_H

#include <metrics/gpu/gpu.h>

void gpu_oneapi_load(gpu_ops_t *ops);

void gpu_oneapi_get_api(uint *api);

state_t gpu_oneapi_init(ctx_t *c);

state_t gpu_oneapi_dispose(ctx_t *c);

state_t gpu_oneapi_count_devices(ctx_t *c, uint *devs_count);

state_t gpu_oneapi_pool(void *p);

state_t gpu_oneapi_read(ctx_t *c, gpu_t *data);

state_t gpu_oneapi_read_raw(ctx_t *c, gpu_t *data);

#endif