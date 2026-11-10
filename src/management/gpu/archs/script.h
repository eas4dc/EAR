/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
#ifndef METRICS_GPU_SCRIPT
#define METRICS_GPU_SCRIPT

#include <management/gpu/gpu.h>

void mgt_gpu_script_load(mgt_gpu_ops_t *ops);

state_t mgt_gpu_script_dispose(ctx_t *c);

state_t mgt_gpu_script_freq_limit_reset(ctx_t *c);

state_t mgt_gpu_script_freq_limit_set(ctx_t *c, ulong *khz);

state_t mgt_gpu_script_power_cap_reset(ctx_t *c);

state_t mgt_gpu_script_power_cap_set(ctx_t *c, uint32_t *watts);

#endif // METRICS_GPU_SCRIPT