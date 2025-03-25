/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//#define SHOW_DEBUGS 1
#include <common/output/debug.h>
#include <management/gpu/gpu.h>
#include <management/gpu/archs/eard.h>
#include <management/gpu/archs/nvml.h>
#include <management/gpu/archs/dummy.h>
#include <management/gpu/archs/oneapi.h>
#include <management/gpu/archs/mgt_pvc_power_hwmon.h>
#include <management/gpu/archs/rsmi.h>

#ifndef USE_PVC_HWMON
#warning "USE_PVC_HWMON not defined! Defining it to 0..."
#define USE_PVC_HWMON 0
#endif

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static mgt_gpu_ops_t   ops;
static uint            loaded;
static ulong          *generic_list;
static uint            devs_count;

__attribute__((used)) void mgt_gpu_load(int force_api)
{
    while (pthread_mutex_trylock(&lock));
    if (loaded) {
        goto unlock_load;
    }
    // NVML and ONEAPI can be partially loaded.
    // EARD then can be partially loaded.
    mgt_gpu_nvml_load(&ops);
    mgt_gpu_rsmi_load(&ops);
		if (USE_PVC_HWMON) {
			mgt_gpu_pvc_hwmon_load(&ops);
		} else {
			mgt_gpu_oneapi_load(&ops);
		}
    mgt_gpu_eard_load(&ops, API_IS(force_api, API_EARD));
    mgt_gpu_dummy_load(&ops);
    loaded = 1;
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

state_t mgt_gpu_freq_limit_reset_dev(ctx_t *c, int gpu)
{
    if (generic_list == NULL) {
        mgt_gpu_data_alloc(&generic_list);
    }
    mgt_gpu_freq_limit_get_default(c, generic_list);
    return mgt_gpu_freq_limit_set_dev(c, generic_list[gpu], gpu);
}

state_t mgt_gpu_freq_limit_set(ctx_t *c, ulong *khz)
{
	preturn (ops.freq_limit_set, c, khz);
}

state_t mgt_gpu_freq_limit_set_dev(ctx_t *c, ulong freq, int gpu)
{
    // Check 0s
    if (generic_list == NULL) {
        mgt_gpu_data_alloc(&generic_list);
    }
    mgt_gpu_data_null(generic_list);
    generic_list[gpu] = freq;
    return mgt_gpu_freq_limit_set(c, generic_list);
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
  debug("mgt_gpu_freq_get_next IN: dev %d freq_ref %lu flag %d FREQ_TOP %d", dev, freq_ref, flag, FREQ_TOP);
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
    debug("i: %d freq_ref %lu freq_ceil %lu freq_floor %lu", i, freq_ref, freq_ceil, freq_floor);
		//
		if (freq_ref == freq_ceil || (freq_ref < freq_ceil && freq_ref > freq_floor)) {
			if (flag == FREQ_TOP) {
				//*freq_idx = i-1;
				*freq_idx = i;
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

state_t mgt_gpu_power_cap_reset_dev(ctx_t *c, int gpu)
{
    if (generic_list == NULL) {
        mgt_gpu_data_alloc(&generic_list);
    }
    mgt_gpu_power_cap_get_default(c, generic_list);
    return mgt_gpu_power_cap_set_dev(c, generic_list[gpu], gpu);
}

state_t mgt_gpu_power_cap_set(ctx_t *c, ulong *watts)
{
	preturn (ops.power_cap_set, c, watts);
}

state_t mgt_gpu_power_cap_set_dev(ctx_t *c, ulong watts, int gpu)
{
    if (generic_list == NULL) {
        mgt_gpu_data_alloc(&generic_list);
    }
    mgt_gpu_data_null(generic_list);
    generic_list[gpu] = watts;
    return mgt_gpu_power_cap_set(c, generic_list);
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

char *mgt_gpu_features_tostr(char *buffer, size_t length)
{
    ulong list_max[128];
    ulong list_min[128];
    const ulong **list;
    const uint *count;
    int i, j = 0;

    mgt_gpu_freq_get_available(no_ctx, &list, &count);
    for (i = 0; i < devs_count; ++i) {
        j += sprintf(&buffer[j], "GPU%d: %4lu to %4lu MHz\n", i, list[i][0], list[i][count[i]-1]);
    }
    mgt_gpu_power_cap_get_rank(no_ctx, list_min, list_max);
    for (i = 0; i < devs_count; ++i) {
        j += sprintf(&buffer[j], "GPU%d: %4lu to %4lu W\n", i, list_max[i], list_min[i]);
    }
    return buffer;
}
