/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef MANAGEMENT_GPU
#define MANAGEMENT_GPU

#include <common/plugins.h>
#include <common/states.h>
#include <common/system/time.h>
#include <common/types.h>
#include <metrics/gpu/gpu.h>

// The API
//
// This is an API to control the frequency and power of the node GPUs.
//
// Props:
// 	- Thread safe: yes.
//	- User mode: no.
//
// Folders:
//	- archs: different GPU controllers, such as Nvidia NMVL.
//
// Future work:
//	- NVML can be compiled without CUDA include, just redefining its symbols in
//	  our nvml.h.
//	- Add a reset function per device (ref: mgt_gpu_freq_limit_reset).
//	- Add a set limit function per device (ref: mgt_gpu_freq_limit_set).
//
// Use example:
//		mgt_gpu_load(NULL);
//		mgt_gpu_init(&context);
//		mgt_gpu_alloc_array(&context, &freq_khz, &dev_count);
// 		mgt_gpu_freq_limit_get_current(&context, freq_khz);
//		mgt_gpu_freq_list(&context, &list_khz, &list_len);
//		for (d = 0; i < &dev_count; ++d)
//			for (f = 0; f < list_len[d]; ++f)
//				printf("freq %lu khz\n", list_khz[d][f]);
//		mgt_gpu_dispose(&context);

// Flags
#define FREQ_TOP    0
#define FREQ_BOTTOM 1

// APIs

typedef struct mgt_gpu_ops_s {

    void (*get_api)(uint *api);
    state_t (*init[4])(ctx_t *c);
    state_t (*dispose[4])(ctx_t *c);
    state_t (*get_devices)(ctx_t *c, gpu_devs_t **devs, uint *devs_count);
    state_t (*count_devices)(ctx_t *c, uint *dev_count);
    state_t (*alloc_array)(ctx_t *c, ulong **list, uint *dev_count);
    state_t (*freq_limit_get_current)(ctx_t *c, ulong *freq_list);
    state_t (*freq_limit_get_default)(ctx_t *c, ulong *freq_list);
    state_t (*freq_limit_get_max)(ctx_t *c, ulong *freq_list);
    state_t (*freq_limit_reset)(ctx_t *c);
    state_t (*freq_limit_set)(ctx_t *c, ulong *freq_list);
    state_t (*freq_list)(ctx_t *c, const ulong ***freq_list, const uint **len_list);
    state_t (*power_cap_get_current)(ctx_t *c, ulong *watt_list);
    state_t (*power_cap_get_default)(ctx_t *c, ulong *watt_list);
    state_t (*power_cap_get_rank)(ctx_t *c, ulong *watt_list_min, ulong *watt_list_max);
    state_t (*power_cap_reset)(ctx_t *c);
    state_t (*power_cap_set)(ctx_t *c, ulong *watt_list);
} mgt_gpu_ops_t;

// Discovers the low level API. Returns function pointers, but is not required.
void mgt_gpu_load(int force_api);

void mgt_gpu_get_api(uint *api);

// Initializes the context.
state_t mgt_gpu_init(ctx_t *c);

state_t mgt_gpu_dispose(ctx_t *c);

// Information about devices
state_t mgt_gpu_get_devices(ctx_t *c, gpu_devs_t **devs, uint *devs_count);

// Counts the number of GPUs (devices) in the node.
state_t mgt_gpu_count_devices(ctx_t *c, uint *dev_count);

// Gets the current clock cap for each GPU in the node (in KHz).
state_t mgt_gpu_freq_limit_get_current(ctx_t *c, ulong *freq_list);

// Gets the default clock cap for each GPU in the node (in KHz).
state_t mgt_gpu_freq_limit_get_default(ctx_t *c, ulong *freq_list);

// Gets the maximum clock cap for each GPU in the node (in KHz).
state_t mgt_gpu_freq_limit_get_max(ctx_t *c, ulong *freq_list);

// Resets the current clock cap on each GPU to its default value.
state_t mgt_gpu_freq_limit_reset(ctx_t *c);

state_t mgt_gpu_freq_limit_reset_dev(ctx_t *c, int gpu);

// Sets the current clock cap on each GPU (one value per GPU in the KHz array).
state_t mgt_gpu_freq_limit_set(ctx_t *c, ulong *freq_list);

state_t mgt_gpu_freq_limit_set_dev(ctx_t *c, ulong freq, int gpu);

// Given a GPU index and reference frequency, get the nearest valid (in khz).
state_t mgt_gpu_freq_get_valid(ctx_t *c, uint dev, ulong freq_ref, ulong *freq_near);

// Given a GPU index, a flag and a reference frequency, get the next frequency index in the list.
state_t mgt_gpu_freq_get_next(ctx_t *c, uint dev, ulong freq_ref, uint *freq_idx, uint flag);

// Gets a list of clocks and list length per device.
state_t mgt_gpu_freq_get_available(ctx_t *c, const ulong ***freq_list, const uint **len_list);

// Gets the current power cap for each GPU in the node.
state_t mgt_gpu_power_cap_get_current(ctx_t *c, ulong *watt_list);

// Gets the default power cap for each GPU in the node.
state_t mgt_gpu_power_cap_get_default(ctx_t *c, ulong *watt_list);

// Gets the minimum/maximum possible power cap for each GPU in the node.
state_t mgt_gpu_power_cap_get_rank(ctx_t *c, ulong *watt_list_min, ulong *watt_list_max);

// Resets the current power cap on each GPU to its default value.
state_t mgt_gpu_power_cap_reset(ctx_t *c);

state_t mgt_gpu_power_cap_reset_dev(ctx_t *c, int gpu);

// Sets the current power cap on each GPU (one value per GPU in the watts array).
state_t mgt_gpu_power_cap_set(ctx_t *c, ulong *watt_list);

state_t mgt_gpu_power_cap_set_dev(ctx_t *c, ulong watts, int gpu);

// Allocates an array of watts or clocks (one per device).
state_t mgt_gpu_data_alloc(ulong **list);

state_t mgt_gpu_data_free(ulong **list);

state_t mgt_gpu_data_null(ulong *list);

int mgt_gpu_is_supported();

char *mgt_gpu_features_tostr(char *buffer, size_t length);

// Renamed functions
#define mgt_gpu_freq_list mgt_gpu_freq_get_available

#endif
