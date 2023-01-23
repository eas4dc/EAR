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

#ifndef METRICS_GPU_CUPTI_H
#define METRICS_GPU_CUPTI_H

#include <metrics/gpu/gpuproc.h>

void gpu_cupti_load(gpuproc_ops_t *ops);

void gpu_cupti_get_api(uint *api);

state_t gpu_cupti_init(ctx_t *c);

state_t gpu_cupti_dispose(ctx_t *c);

state_t gpu_cupti_count_devices(ctx_t *c, uint *devs_count);

state_t gpu_cupti_pool(void *p);

state_t gpu_cupti_read(ctx_t *c, gpuproc_t *data);

state_t gpu_cupti_enable(ctx_t *c);

state_t gpu_cupti_disable(ctx_t *c);

#endif //METRICS_GPU_CUPTI_H
