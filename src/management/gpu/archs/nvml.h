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

#ifndef MANAGEMENT_GPU_NVML
#define MANAGEMENT_GPU_NVML

#include <management/gpu/gpu.h>

state_t mgt_nvml_status();

state_t mgt_nvml_init(ctx_t *c);

state_t mgt_nvml_init_unprivileged(ctx_t *c);

state_t mgt_nvml_dispose(ctx_t *c);

state_t mgt_nvml_count(ctx_t *c, uint *dev_count);

state_t nvml_freq_limit_get_current(ctx_t *c, ulong *khz);

state_t nvml_freq_limit_get_default(ctx_t *c, ulong *khz);

state_t nvml_freq_limit_get_max(ctx_t *c, ulong *khz);

state_t nvml_freq_limit_reset(ctx_t *c);

state_t nvml_freq_limit_set(ctx_t *c, ulong *khz);

state_t nvml_freq_get_valid(ctx_t *c, uint dev, ulong freq_ref, ulong *freq_near);

state_t nvml_freq_get_next(ctx_t *c, uint dev, ulong freq_ref, uint *freq_idx, uint flag);

state_t nvml_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len);

state_t nvml_power_cap_get_current(ctx_t *c, ulong *watts);

state_t nvml_power_cap_get_default(ctx_t *c, ulong *watts);

state_t nvml_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max);

state_t nvml_power_cap_reset(ctx_t *c);

state_t nvml_power_cap_set(ctx_t *c, ulong *watts);

#endif