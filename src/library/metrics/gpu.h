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

#ifndef LIBRARY_METRICS_GPU_H
#define LIBRARY_METRICS_GPU_H

#include <metrics/gpu/gpu.h>
#include <management/gpu/gpu.h>
#include <daemon/shared_configuration.h>

state_t gpu_lib_load(settings_conf_t *settings);

state_t gpu_lib_init(ctx_t *c);

state_t gpu_lib_dispose(ctx_t *c);

/* If not loaded, model is undefined */
state_t gpu_lib_model(ctx_t *_c,uint *model);

/* Metrics. */
state_t gpu_lib_count(uint *dev_count);

// The ctx_t is obsolete.
state_t gpu_lib_read(ctx_t *c, gpu_t *data);

state_t gpu_lib_read_raw(ctx_t *c, gpu_t *data);

state_t gpu_lib_read_copy(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

state_t gpu_lib_data_diff(gpu_t *data2, gpu_t *data1, gpu_t *data_diff);

state_t gpu_lib_data_merge(gpu_t *data_diff, gpu_t *data_merge);

state_t gpu_lib_data_alloc(gpu_t **data);

state_t gpu_lib_data_free(gpu_t **data);

state_t gpu_lib_data_null(gpu_t *data);

state_t gpu_lib_data_copy(gpu_t *data_dst, gpu_t *data_src);

state_t gpu_lib_data_print(gpu_t *data, int fd);

state_t gpu_lib_data_tostr(gpu_t *data, char *buffer, int length);

/* Management. */
state_t gpu_lib_alloc_array(ulong **array);

state_t gpu_lib_freq_limit_get_current(ctx_t *c, ulong *khz);

state_t gpu_lib_freq_limit_get_default(ulong *khz);

state_t gpu_lib_freq_limit_get_max(ulong *khz);

state_t gpu_lib_freq_limit_reset();

state_t gpu_lib_freq_limit_set(ulong *khz);

state_t gpu_lib_freq_list(const ulong ***list_khz, const uint **list_len);

state_t gpu_lib_power_cap_get_current(ctx_t *c, ulong *watts);

state_t gpu_lib_power_cap_get_default(ulong *watts);

state_t gpu_lib_power_cap_get_rank(ulong *watts_min, ulong *watts_max);

state_t gpu_lib_power_cap_reset();

state_t gpu_lib_power_cap_set(ulong *watts);

#endif
