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

//#define SHOW_DEBUGS 1

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <management/gpu/gpu.h>
#include <management/gpu/archs/eard.h>
#include <management/gpu/archs/nvml.h>
#include <management/gpu/archs/dummy.h>
#include <management/gpu/archs/oneapi.h>
#include <management/gpu/archs/rsmi.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static mgt_gpu_ops_t ops;
static uint devs_count;
static uint ok_load;

__attribute__((used)) void mgt_gpu_load(int eard)
{
    while (pthread_mutex_trylock(&lock));
    if (ok_load) {
        goto unlock_load;
    }
    // NVML and ONEAPI can be partially loaded.
    // EARD then can be partially loaded.
    mgt_gpu_nvml_load(&ops);
    mgt_gpu_rsmi_load(&ops);
    mgt_gpu_oneapi_load(&ops);
    mgt_gpu_eard_load(&ops, eard);
    mgt_gpu_dummy_load(&ops);
    ok_load = 1;
unlock_load:
    pthread_mutex_unlock(&lock);
}

void mgt_gpu_get_api(uint *api)
{
    *api = API_NONE;
    if (ops.get_api != NULL) {
        return ops.get_api(api);
    }
}

__attribute__((used)) state_t mgt_gpu_init(ctx_t *c)
{
    apis_multi(ops.init, c);
    return mgt_gpu_count_devices(c, &devs_count);
}

__attribute__((used)) state_t mgt_gpu_dispose(ctx_t *c)
{
    apis_multiret(ops.dispose, c);
}

state_t mgt_gpu_get_devices(ctx_t *c, gpu_devs_t **devs, uint *devs_count)
{
    preturn (ops.get_devices, c, devs, devs_count);
}

__attribute__((used)) state_t mgt_gpu_count_devices(ctx_t *c, uint *dev_count)
{
	preturn (ops.count_devices, c, dev_count);
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

state_t mgt_gpu_freq_get_valid(ctx_t *c, uint dev, ulong freq_ref, ulong *freq_near)
{
	const uint   *freqs_alldevs_count;
	const ulong **freqs_alldevs;
	const ulong  *freqs_dev;
	uint          freqs_dev_count;
	ulong         freq_floor;
	ulong         freq_ceil;
	state_t       s;
	uint          i;

	if (state_fail(s = mgt_gpu_freq_list(c, &freqs_alldevs, &freqs_alldevs_count))) {
		return s;
	}
	// Indexing
	freqs_dev_count = freqs_alldevs_count[dev];
	freqs_dev = freqs_alldevs[dev];
	//
	if (freq_ref > freqs_dev[0]) {
		*freq_near = freqs_dev[0];
		return EAR_SUCCESS;
	}
	for (i = 0; i < freqs_dev_count-1; ++i)
	{
		freq_ceil  = freqs_dev[i+0];
		freq_floor = freqs_dev[i+1];

		if (freq_ref <= freq_ceil && freq_ref >= freq_floor) {
			if ((freq_ceil - freq_ref) <= (freq_ref - freq_floor)) {
				*freq_near = freq_ceil;
			} else {
				*freq_near = freq_floor;
			}
			return EAR_SUCCESS;
		}
	}
	*freq_near = freqs_dev[i];

	return EAR_SUCCESS;
}

state_t mgt_gpu_freq_get_next(ctx_t *c, uint dev, ulong freq_ref, uint *freq_idx, uint flag)
{
	const uint   *freqs_alldevs_count;
	const ulong **freqs_alldevs;
	const ulong  *freqs_dev;
	uint          freqs_dev_count;
	ulong         freq_floor;
	ulong         freq_ceil;
	state_t       s;
	uint          i;

	if (state_fail(s = mgt_gpu_freq_get_available(c, &freqs_alldevs, &freqs_alldevs_count))) {
		return s;
	}
	// Indexing
	freqs_dev_count = freqs_alldevs_count[dev];
	freqs_dev = freqs_alldevs[dev];
	//
	if (freq_ref >= freqs_dev[0]) {
		if (flag == FREQ_TOP) {
			*freq_idx = 0;
		} else {
			*freq_idx = 1;
		}
		return EAR_SUCCESS;
	}
	for (i = 1; i < freqs_dev_count-1; ++i)
	{
		freq_ceil  = freqs_dev[i+0];
		freq_floor = freqs_dev[i+1];
		//
		if (freq_ref == freq_ceil || (freq_ref < freq_ceil && freq_ref > freq_floor)) {
			if (flag == FREQ_TOP) {
				*freq_idx = i-1;
			} else {
				*freq_idx = i+1;
			}
			return EAR_SUCCESS;
		}
	}
	if (flag == FREQ_TOP) {
		*freq_idx = i-1;
	} else {
		*freq_idx = i;
	}
	return EAR_SUCCESS;
}

state_t mgt_gpu_freq_get_available(ctx_t *c, const ulong ***list_khz, const uint **list_len)
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

state_t mgt_gpu_data_alloc(ulong **data)
{
    if (data == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    }
    *data = calloc(devs_count, sizeof(ulong));
    if (*data == NULL) {
        return_msg(EAR_ERROR, strerror(errno));
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_data_free(ulong **data)
{
    if (data != NULL) {
        free(*data);
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_data_null(ulong *data)
{
    if (data == NULL) {
        return_msg(EAR_ERROR, Generr.input_null);
    }
    memset(data, 0, devs_count * sizeof(ulong));
    return EAR_SUCCESS;
}

int mgt_gpu_is_supported()
{
    return mgt_gpu_eard_is_supported();
}
