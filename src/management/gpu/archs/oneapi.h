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

#ifndef MANAGEMENT_GPU_ONEAPI_H
#define MANAGEMENT_GPU_ONEAPI_H

#include <management/gpu/gpu.h>

void mgt_gpu_oneapi_load(mgt_gpu_ops_t *ops);

void mgt_gpu_oneapi_get_api(uint *api);

state_t mgt_gpu_oneapi_init(ctx_t *c);

state_t mgt_gpu_oneapi_dispose(ctx_t *c);

state_t mgt_gpu_oneapi_get_devices(ctx_t *c, gpu_devs_t **devs, uint *devs_count);

state_t mgt_gpu_oneapi_count_devices(ctx_t *c, uint *dev_count);

state_t mgt_gpu_oneapi_freq_limit_get_current(ctx_t *c, ulong *khz);

state_t mgt_gpu_oneapi_freq_limit_get_default(ctx_t *c, ulong *khz);

state_t mgt_gpu_oneapi_freq_limit_get_max(ctx_t *c, ulong *khz);

state_t mgt_gpu_oneapi_freq_limit_reset(ctx_t *c);

state_t mgt_gpu_oneapi_freq_limit_set(ctx_t *c, ulong *khz);

state_t mgt_gpu_oneapi_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len);

state_t mgt_gpu_oneapi_power_cap_get_current(ctx_t *c, ulong *watts);

state_t mgt_gpu_oneapi_power_cap_get_default(ctx_t *c, ulong *watts);

state_t mgt_gpu_oneapi_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max);

state_t mgt_gpu_oneapi_power_cap_reset(ctx_t *c);

state_t mgt_gpu_oneapi_power_cap_set(ctx_t *c, ulong *watts);

#endif //MANAGEMENT_GPU_ONEAPI_H
