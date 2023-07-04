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

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/config/config_env.h>
#include <metrics/common/oneapi.h>
#include <management/gpu/archs/oneapi.h>

//static pthread_mutex_t      lock = PTHREAD_MUTEX_INITIALIZER;
static ulong             *freqs_default;
static ulong            **freqs_avail_list;
static uint              *freqs_avail_count;
static ulong             *power_max_default;
static uint               devs_count;
static ze_handlers_t     *devs;
static ze_t               ze;

#define ret_fail(function) \
    if ((z = function) != ZE_RESULT_SUCCESS) { \
        debug("Failed " #function ": %s (0x%x)", oneapi_strerror(z), z); \
        return EAR_ERROR; \
    }

static state_t get_available_watts()
{
    zes_power_sustained_limit_t ps;
    zes_power_burst_limit_t     pb;
    zes_power_peak_limit_t      pp;
    uint                        dv;
    uint                        d;
    ze_result_t                 z;

    power_max_default = calloc(devs_count, sizeof(uint));
    // Retrieving the domains
    for (dv = 0; dv < devs_count; ++dv){
        for (d = 0; d < devs[dv].spowers_count; ++d) {
            ret_fail(ze.sPowerGetLimits(devs[dv].spowers[d], &ps, &pb, &pp));
            power_max_default[dv] = ((ulong) ps.power) / 1000LU; // mW
            debug("-- DEVICE%d POWER DOMAIN%d ---------------", dv, d);
            debug("defaultLimit: %u", props.defaultLimit);
            debug("maxLimit: %u", props.maxLimit);
            debug("minLimit: %u", props.minLimit);
            debug("sustained.enabled: %u", ps.enabled);
            debug("sustained.power: %u", ps.power);
            debug("sustained.interval: %u", ps.interval);
            debug("burst.enabled: %u", pb.enabled);
            debug("burst.power: %u", pb.power);
            debug("peak powerAC: %u", pp.powerAC);
            debug("peak powerDC: %u", pp.powerDC);
            debug("----------------------------------------");
        }
    }
    return EAR_SUCCESS;
}

static state_t get_available_frequencies()
{
    double                avail_aux[1024];
    zes_freq_range_t      range;
    uint                  dv;
    uint                  d;
    uint                  f;
    ze_result_t           z;

    // Allocating devices counters and pointers
    freqs_avail_count = calloc(devs_count, sizeof(uint));
    freqs_avail_list  = calloc(devs_count, sizeof(ulong *));
    freqs_default     = calloc(devs_count, sizeof(ulong));
    // Retrieving the domains
    for (dv = 0; dv < devs_count; ++dv) {
        // Iterating over domains
        for (d = 0; d < devs[dv].sfreqs_count; ++d) {
            // If is not GPU, is not interesting
            if (devs[dv].sfreqs_props[d].type != ZES_FREQ_DOMAIN_GPU) {
                continue;
            }
            // Saving the default frequencies
            freqs_avail_count[dv] = 0;
            ret_fail(ze.sFrequencyGetAvailableClocks(devs[dv].sfreqs[d], &freqs_avail_count[dv], NULL));
            freqs_avail_list[dv] = calloc(freqs_avail_count[dv], sizeof(ulong));
            ret_fail(ze.sFrequencyGetAvailableClocks(devs[dv].sfreqs[d], &freqs_avail_count[dv], avail_aux));
            // Converting double to ulong
            for (f = 0; f < freqs_avail_count[dv]; ++f) {
                freqs_avail_list[dv][f] = ((ulong) avail_aux[freqs_avail_count[dv]-f-1]) * 1000LU;
            }
            ret_fail(ze.sFrequencyGetRange(devs[dv].sfreqs[d], &range));
            freqs_default[dv] = (ulong) range.max * 1000LU; //MHz
            debug("-- DEVICE%d FREQS DOMAIN%d ---------------", dv, d);
            debug("ps0: %lu KHz", freqs_avail_list[dv][0]);
            debug("ps%d: %lu KHz", freqs_avail_count[dv]-1, freqs_avail_list[dv][freqs_avail_count[dv]-1]);
            debug("ps_default: %lu KHz", freqs_default[dv]);
            debug("----------------------------------------");
        }
    }
    return EAR_SUCCESS;
}

void mgt_gpu_oneapi_load(mgt_gpu_ops_t *ops)
{
    if (state_fail(oneapi_open(&ze))) {
        debug("oneapi_open failed: %s", state_msg);
        return;
    }
    if (state_fail(oneapi_get_devices(&devs, &devs_count))) {
        debug("oneapi_get_devices failed: %s", state_msg);
        return;
    }
    // Get static values
    get_available_frequencies();
    get_available_watts();
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
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_get_devices(ctx_t *c, gpu_devs_t **devs_in, uint *devs_count_in)
{
    int dv;
    *devs_in = calloc(devs_count, sizeof(gpu_devs_t));
    //
    for (dv = 0; dv < devs_count; ++dv) {
        (*devs_in)[dv].serial = devs[dv].serial;
        (*devs_in)[dv].index  = dv;
    }
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_count_devices(ctx_t *c, uint *devs_count_in)
{
    *devs_count_in = devs_count;
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_get_current(ctx_t *c, ulong *khz)
{
    //zes_freq_range_t range;
    //ze_result_t z;
    int dv;
    for (dv = 0; dv < devs_count; ++dv) {
        //ret_fail(ze.sFrequencyGetRange(devs_freq[dv], &range));
        //khz[dv] = (ulong) range.max * 1000LU;
        khz[dv] = freqs_default[dv];
    }
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_get_default(ctx_t *c, ulong *khz)
{
    int dv;
    for (dv = 0; dv < devs_count; ++dv) {
        khz[dv] = freqs_default[dv];
    }
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_get_max(ctx_t *c, ulong *khz)
{
    int dv;
    for (dv = 0; dv < devs_count; ++dv) {
        khz[dv] = freqs_avail_list[dv][0];
    }
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_reset(ctx_t *c)
{
    // Nothing by now, the function will be zesPowerGetLimits, but can't be tested yet.
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_set(ctx_t *c, ulong *khz)
{
    // Nothing by now, the function will be zesPowerGetLimits, but can't be tested yet.
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
    if (list_khz != NULL) {
        *list_khz = (const ulong **) freqs_avail_list;
    }
    if (list_len != NULL) {
        *list_len = (const uint *) freqs_avail_count;
    }
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_get_current(ctx_t *c, ulong *watts)
{
    zes_power_sustained_limit_t ps;
    zes_power_burst_limit_t     pb;
    zes_power_peak_limit_t      pp;
    ze_result_t                 z;
    int                         dv;

    for (dv = 0; dv < devs_count; ++dv) {
        watts[dv] = 0LU;
        if (devs[dv].spowers_count) {
            ret_fail(ze.sPowerGetLimits(devs[dv].spowers[0], &ps, &pb, &pp));
            watts[dv] = ((ulong) ps.power) / 1000LU; // mW
        }
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_get_default(ctx_t *c, ulong *watts)
{
    int dv;
    for (dv = 0; dv < devs_count; ++dv) {
        watts[dv] = power_max_default[dv];
    }
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
    int dv;
    if (watts_max != NULL) {
        // Going by default because boost and peak are
        mgt_gpu_oneapi_power_cap_get_default(c, watts_max);
    }
    for (dv = 0; dv < devs_count; ++dv) {
        watts_min[dv] = 0LU;
    }
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_reset(ctx_t *c)
{
    //
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_set(ctx_t *c, ulong *watts)
{
    //
    return EAR_SUCCESS;
}