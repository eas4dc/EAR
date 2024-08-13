/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_GPU_RSMI_H
#define METRICS_GPU_RSMI_H

#include <metrics/gpu/gpu.h>

void gpu_rsmi_load(gpu_ops_t *ops, int eard);

void gpu_rsmi_get_info(apinfo_t *info);

void gpu_rsmi_get_devices(gpu_devs_t **devs, uint *devs_count);

state_t gpu_rsmi_init(ctx_t *c);

state_t gpu_rsmi_dispose(ctx_t *c);

state_t gpu_rsmi_pool(void *c);

state_t gpu_rsmi_read(ctx_t *c, gpu_t *data);

/* Reads the data directly from the GPU API (not preprocessed data). */
state_t gpu_rsmi_read_raw(ctx_t *c, gpu_t *data);

#endif
