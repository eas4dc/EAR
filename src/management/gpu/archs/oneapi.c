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

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/config/config_env.h>
#include <metrics/common/oneapi.h>
#include <management/gpu/archs/oneapi.h>

//static pthread_mutex_t      lock = PTHREAD_MUTEX_INITIALIZER;
static zes_device_handle_t *devs;
static uint			        devs_count;
static ze_t                 ze;

void mgt_gpu_oneapi_load(mgt_gpu_ops_t *ops)
{
    #if 1
    return;
    #endif
    if (state_fail(oneapi_open(&ze))) {
        debug("oneapi_open failed: %s", state_msg);
        return;
    }
    if (state_fail(oneapi_get_devices(&devs, &devs_count))) {
        debug("oneapi_get_devices failed: %s", state_msg);
        return;
    }
    // Set always
    apis_set(ops->init                  , mgt_gpu_oneapi_init);
    apis_set(ops->dispose               , mgt_gpu_oneapi_dispose);
    // Queries
    apis_set(ops->get_api               , mgt_gpu_oneapi_get_api);
    apis_set(ops->get_devices,            mgt_gpu_oneapi_get_devices);
    apis_set(ops->count_devices         , mgt_gpu_oneapi_count_devices);
    apis_set(ops->freq_limit_get_current, mgt_gpu_oneapi_freq_limit_get_current);
    apis_set(ops->freq_limit_get_default, mgt_gpu_oneapi_freq_limit_get_default);
    apis_set(ops->freq_limit_get_max    , mgt_gpu_oneapi_freq_limit_get_max);
    apis_set(ops->freq_list             , mgt_gpu_oneapi_freq_list);
    apis_set(ops->power_cap_get_current , mgt_gpu_oneapi_power_cap_get_current);
    apis_set(ops->power_cap_get_default , mgt_gpu_oneapi_power_cap_get_default);
    apis_set(ops->power_cap_get_rank    , mgt_gpu_oneapi_power_cap_get_rank);
    // Checking if NVML is capable to run commands
    if (!oneapi_is_privileged()) {
        return;
    }
    // Commands
    apis_set(ops->freq_limit_reset      , mgt_gpu_oneapi_freq_limit_reset);
    apis_set(ops->freq_limit_set        , mgt_gpu_oneapi_freq_limit_set);
    apis_set(ops->power_cap_reset       , mgt_gpu_oneapi_power_cap_reset);
    apis_set(ops->power_cap_set         , mgt_gpu_oneapi_power_cap_set);
}

void mgt_gpu_oneapi_get_api(uint *api)
{
    *api = API_ONEAPI;
}

state_t mgt_gpu_oneapi_init(ctx_t *c)
{
    // A lot of statics
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_get_devices(ctx_t *c, gpu_devs_t **devs, uint *devs_count)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_count_devices(ctx_t *c, uint *devs_count_in)
{
    *devs_count_in = devs_count;
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_get_current(ctx_t *c, ulong *khz)
{
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_get_default(ctx_t *c, ulong *khz)
{
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_get_max(ctx_t *c, ulong *khz)
{
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_reset(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_set(ctx_t *c, ulong *khz)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_get_current(ctx_t *c, ulong *watts)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_get_default(ctx_t *c, ulong *watts)
{
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_reset(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_set(ctx_t *c, ulong *watts)
{
    return EAR_SUCCESS;
}
