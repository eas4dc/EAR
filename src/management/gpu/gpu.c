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
//#define SHOW_DEBUGS 1
#include <stdio.h>
#include <stdlib.h>
#include <common/output/debug.h>
#include <management/gpu/gpu.h>
#include <management/gpu/archs/nvml.h>
#include <management/gpu/archs/dummy.h>

static mgt_gpu_ops_t ops;
static uint loaded;

__attribute__((used)) state_t mgt_gpu_load(mgt_gpu_ops_t **_ops)
{

	debug("mgt_gpu_load init");
	if (loaded != 0) {
		if (_ops != NULL) {
			*_ops = &ops;
		}
		return EAR_SUCCESS;
	}

	if (state_ok(mgt_nvml_status())) {
		debug("loaded NVML");
		ops.init                   = mgt_nvml_init;
		ops.init_unprivileged      = mgt_nvml_init_unprivileged;
		ops.dispose                = mgt_nvml_dispose;
		ops.count                  = mgt_nvml_count;
		ops.alloc_array            = mgt_gpu_alloc_array;
		ops.freq_limit_get_current = nvml_freq_limit_get_current;
		ops.freq_limit_get_default = nvml_freq_limit_get_default;
		ops.freq_limit_get_max     = nvml_freq_limit_get_max;
		ops.freq_limit_reset       = nvml_freq_limit_reset;
		ops.freq_limit_set         = nvml_freq_limit_set;
		ops.freq_get_valid         = nvml_freq_get_valid;
		ops.freq_get_next          = nvml_freq_get_next;
		ops.freq_list              = nvml_freq_list;
		ops.power_cap_get_current  = nvml_power_cap_get_current;
		ops.power_cap_get_default  = nvml_power_cap_get_default;
		ops.power_cap_get_rank     = nvml_power_cap_get_rank;
		ops.power_cap_reset        = nvml_power_cap_reset;
		ops.power_cap_set          = nvml_power_cap_set;
	} else {
		debug("loaded DUMMY");
		ops.init                   = mgt_dummy_init;
		ops.init_unprivileged      = mgt_dummy_init;
		ops.dispose                = mgt_dummy_dispose;
		ops.count                  = mgt_dummy_count;
		ops.alloc_array            = mgt_gpu_alloc_array;
		ops.freq_limit_get_current = dummy_freq_limit_get_current;
		ops.freq_limit_get_default = dummy_freq_limit_get_default;
		ops.freq_limit_get_max     = dummy_freq_limit_get_max;
		ops.freq_limit_reset       = dummy_freq_limit_reset;
		ops.freq_limit_set         = dummy_freq_limit_set;
		ops.freq_get_valid         = dummy_freq_get_valid;
		ops.freq_get_next          = dummy_freq_get_next;
		ops.freq_list              = dummy_freq_list;
		ops.power_cap_get_current  = dummy_power_cap_get_current;
		ops.power_cap_get_default  = dummy_power_cap_get_default;
		ops.power_cap_get_rank     = dummy_power_cap_get_rank;
		ops.power_cap_reset        = dummy_power_cap_reset;
		ops.power_cap_set          = dummy_power_cap_set;
	}
	if (_ops != NULL) {
		*_ops = &ops;
	}
	// Dummy loads anyway
	loaded = 1;

	return EAR_SUCCESS;
}

__attribute__((used)) state_t mgt_gpu_init(ctx_t *c)
{
	preturn (ops.init, c);
}

__attribute__((used)) state_t mgt_gpu_dispose(ctx_t *c)
{
	preturn (ops.dispose, c);
}

__attribute__((used)) state_t mgt_gpu_count(ctx_t *c, uint *dev_count)
{
	preturn (ops.count, c, dev_count);
}

state_t mgt_gpu_alloc_array(ctx_t *c, ulong **array, uint *_dev_count)
{
	uint dev_count;
	mgt_gpu_count(c, &dev_count);
	*array = calloc(dev_count, sizeof(ulong));
	if (_dev_count != NULL) *_dev_count = dev_count;
	return EAR_SUCCESS;
}

state_t mgt_gpu_freq_limit_get_current(ctx_t *c, ulong *khz)
{
	preturn (ops.freq_limit_get_current, c, khz);
}

state_t mgt_gpu_freq_limit_get_default(ctx_t *c, ulong *khz)
{
	preturn (ops.freq_limit_get_default, c, khz);
}

state_t mgt_gpu_freq_limit_get_max(ctx_t *c, ulong *khz)
{
	preturn (ops.freq_limit_get_max, c, khz);
}

state_t mgt_gpu_freq_limit_reset(ctx_t *c)
{
	preturn (ops.freq_limit_reset, c);
}

state_t mgt_gpu_freq_limit_set(ctx_t *c, ulong *khz)
{
	preturn (ops.freq_limit_set, c, khz);
}

state_t mgt_gpu_freq_get_valid(ctx_t *c, uint d, ulong freq_ref, ulong *freq_near)
{
	preturn (ops.freq_get_valid, c, d, freq_ref, freq_near);
}

state_t mgt_gpu_freq_get_next(ctx_t *c, uint d, ulong freq_ref, uint *freq_idx, uint flag)
{
	preturn (ops.freq_get_next, c, d, freq_ref, freq_idx, flag);
}

state_t mgt_gpu_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
	preturn (ops.freq_list, c, list_khz, list_len);
}

state_t mgt_gpu_power_cap_get_current(ctx_t *c, ulong *watts)
{
	preturn (ops.power_cap_get_current, c, watts);
}

state_t mgt_gpu_power_cap_get_default(ctx_t *c, ulong *watts)
{
	preturn (ops.power_cap_get_default, c, watts);
}

state_t mgt_gpu_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
	preturn (ops.power_cap_get_rank, c, watts_min, watts_max);
}

state_t mgt_gpu_power_cap_reset(ctx_t *c)
{
	preturn (ops.power_cap_reset, c);
}

state_t mgt_gpu_power_cap_set(ctx_t *c, ulong *watts)
{
	preturn (ops.power_cap_set, c, watts);
}
