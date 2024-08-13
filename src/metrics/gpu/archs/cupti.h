/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
