/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_GPU_ONEAPI_H
#define METRICS_GPU_ONEAPI_H

#include <metrics/gpu/gpu.h>

void gpu_oneapi_load(gpu_ops_t *ops, int force_api);

void gpu_oneapi_get_info(apinfo_t *info);

void gpu_oneapi_get_devices(gpu_devs_t **devs, uint *devs_count);

state_t gpu_oneapi_init(ctx_t *c);

state_t gpu_oneapi_dispose(ctx_t *c);

void gpu_oneapi_set_monitoring_mode(int mode_in);

state_t gpu_oneapi_pool(void *p);

state_t gpu_oneapi_read(ctx_t *c, gpu_t *data);

state_t gpu_oneapi_read_raw(ctx_t *c, gpu_t *data);

void gpu_oneapi_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

#endif