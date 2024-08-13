/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_GPU_DUMMY
#define METRICS_GPU_DUMMY

#include <management/gpu/gpu.h>

void mgt_gpu_dummy_load(mgt_gpu_ops_t *ops);

void mgt_gpu_dummy_get_api(uint *api);

state_t mgt_gpu_dummy_init(ctx_t *c);

state_t mgt_gpu_dummy_dispose(ctx_t *c);

state_t mgt_gpu_dummy_get_devices(ctx_t *c, gpu_devs_t **devs, uint *devs_count);

state_t mgt_gpu_dummy_count_devices(ctx_t *c, uint *devs_count);

state_t mgt_gpu_dummy_freq_limit_get_current(ctx_t *c, ulong *khz);

state_t mgt_gpu_dummy_freq_limit_get_default(ctx_t *c, ulong *khz);

state_t mgt_gpu_dummy_freq_limit_get_max(ctx_t *c, ulong *khz);

state_t mgt_gpu_dummy_freq_limit_reset(ctx_t *c);

state_t mgt_gpu_dummy_freq_limit_set(ctx_t *c, ulong *khz);

state_t mgt_gpu_dummy_freq_get_available(ctx_t *c, const ulong ***list_khz, const uint **list_len);

state_t mgt_gpu_dummy_power_cap_get_current(ctx_t *c, ulong *watts);

state_t mgt_gpu_dummy_power_cap_get_default(ctx_t *c, ulong *watts);

state_t mgt_gpu_dummy_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max);

state_t mgt_gpu_dummy_power_cap_reset(ctx_t *c);

state_t mgt_gpu_dummy_power_cap_set(ctx_t *c, ulong *watts);

#endif
