/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

//#define SHOW_DEBUGS 1

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <common/config/config_env.h>
#include <metrics/common/oneapi.h>
#include <management/gpu/archs/oneapi.h>

//static pthread_mutex_t           lock = PTHREAD_MUTEX_INITIALIZER;
static ulong                      *freqs_default;
static ulong                     **freqs_avail_list;
static uint                       *freqs_avail_count;
static ulong                      *power_max_default;
static zes_power_limit_ext_desc_t *power_descs; //sustaineds
static uint                       *power_domains;
static uint                        devs_count;
static ze_handlers_t              *devs;
static ze_t                        ze;

#define DUMMY_FREQ  1000000LU
#define NO_FREQ    -1

#define if_fail(function, action) \
    if ((z = function) != ZE_RESULT_SUCCESS) { \
        debug("Failed " #function ": %s (0x%x)", oneapi_strerror(z), z); \
        action; \
    }

static state_t get_available_watts()
{
    uint                        descs_count = 32;
    zes_power_limit_ext_desc_t  descs[32];
    uint                        dv;
    uint                        dm;
    uint                        de;
    ze_result_t                 z;
    #if SHOW_DEBUGS
    char                       *sources[4] __attribute__ ((unused));
    char                       *levels[6]  __attribute__ ((unused));
    char                       *units[4]   __attribute__ ((unused));
    // For debugging
    levels[ZES_POWER_LEVEL_UNKNOWN]        = "UNKNOWN";
    levels[ZES_POWER_LEVEL_SUSTAINED]      = "SUSTAINED";
    levels[ZES_POWER_LEVEL_BURST]          = "BURTS";
    levels[ZES_POWER_LEVEL_PEAK]           = "PEAK";
    levels[ZES_POWER_LEVEL_INSTANTANEOUS]  = "INSTANTANEOUS";
    units[ZES_LIMIT_UNIT_UNKNOWN]          = "of unknown units";
    units[ZES_LIMIT_UNIT_CURRENT]          = "A"; //We are converting from mA to A
    units[ZES_LIMIT_UNIT_POWER]            = "W"; //We are converting from mW to W
    sources[ZES_POWER_SOURCE_ANY]          = "any";
    sources[ZES_POWER_SOURCE_MAINS]        = "main";
    sources[ZES_POWER_SOURCE_BATTERY]      = "any";
    #endif

    power_max_default = calloc(devs_count, sizeof(ulong));
    power_descs       = calloc(devs_count, sizeof(zes_power_limit_ext_desc_t));
    power_domains     = calloc(devs_count, sizeof(uint));
    
    for (dv = 0; dv < devs_count; ++dv) {
        // Dummyfying the power
        power_descs[dv].enabled            = 0;
        power_descs[dv].enabledStateLocked = 1;
        // 
        for (dm = 0; dm < devs[dv].spowers_count; ++dm)
        {
            if_fail(ze.sPowerGetLimitsExt(devs[dv].spowers[dm], &descs_count, descs), break);
            if (descs_count == 0) {
                debug("POWER DEV%d DOM%d DESC0: not found (sub %d)", dv, dm, devs[dv].spowers_props[dm].subdeviceId);
            }
            for (de = 0; de < descs_count; ++de) {
                // Take into the account ps.interval, pb.enabled, pb.power, pp.powerAC and pp.powerDC in future
                debug("POWER DEV%d DOM%d DESC%d: limit %03d %s at %04d mS interval (%d%d, %s, %s) (sub %d)", dv, dm, de, descs[de].limit / 1000,
                        units[descs[de].limitUnit], descs[de].interval, descs[de].limitValueLocked, descs[de].intervalValueLocked,
                        levels[descs[de].level], sources[descs[de].source], devs[dv].spowers_props[dm].subdeviceId);
                // Subdevice -1 is a root device
                if (descs[de].level == ZES_POWER_LEVEL_SUSTAINED && devs[dv].spowers_props[dm].subdeviceId == -1) {
                    power_max_default[dv] = ((ulong) descs[de].limit) / 1000LU;
                    power_domains[dv] = dm;
                    memcpy(&power_descs[dv], &descs[de], sizeof(zes_power_limit_ext_desc_t));
                    #if !SHOW_DEBUGS
                    break;
                    #endif
                }
            }
        }
    }
    return EAR_SUCCESS;
}

static state_t get_available_frequencies()
{
    static ulong          dummy_freq = DUMMY_FREQ;
    double                avail_aux[1024];
    zes_freq_range_t      range;
    uint                  dv;
    uint                  d;
    uint                  f;
    uint                  r; //Real count
    ze_result_t           z;

    // Allocating devices counters and pointers
    freqs_avail_count = calloc(devs_count, sizeof(uint));
    freqs_avail_list  = calloc(devs_count, sizeof(ulong *));
    freqs_default     = calloc(devs_count, sizeof(ulong));
    // Iterating over devices
    for (dv = 0; dv < devs_count; ++dv) {
        // Dummy
        freqs_avail_count[dv] = 1;
        freqs_avail_list[dv]  = &dummy_freq;
        freqs_default[dv]     = NO_FREQ;
        // Iterating over domains
        for (d = 0; d < devs[dv].sfreqs_count; ++d) {
            // If is not GPU, is not interesting. Other types are MEMORY and MEDIA.
            // Multiple GPU domains are possible... And this is weird.
            if (devs[dv].sfreqs_props[d].type != ZES_FREQ_DOMAIN_GPU) {
                continue;
            }
            // Saving the default frequencies. At least from the last GPU domain read.
            // This can be a problem? We have no experience.
            freqs_avail_count[dv] = 0;
            if_fail(ze.sFrequencyGetAvailableClocks(devs[dv].sfreqs[d], &freqs_avail_count[dv], NULL),);
            freqs_avail_list[dv] = calloc(freqs_avail_count[dv], sizeof(ulong));
            if_fail(ze.sFrequencyGetAvailableClocks(devs[dv].sfreqs[d], &freqs_avail_count[dv], avail_aux),);
            // Converting double to ulong
            for (f = r = 0; f < freqs_avail_count[dv]; ++f) {
                ulong aux_integer1 = ((ulong) avail_aux[freqs_avail_count[dv]-f-1]);
                ulong aux_integer2 = (aux_integer1 / 100LU) * 100LU;
                ulong aux_integer3 = aux_integer1 - aux_integer2;

                if (aux_integer3 == 0LU || aux_integer3 == 50LU) {
                    freqs_avail_list[dv][r] = ((ulong) avail_aux[freqs_avail_count[dv]-f-1]) * 1000LU;
//                    debug("DV%d PS%02d: %lu", dv, r, freqs_avail_list[dv][r]);
                    ++r;
                }
            }
            freqs_avail_count[dv] = r;
            if_fail(ze.sFrequencyGetRange(devs[dv].sfreqs[d], &range),);
            freqs_default[dv] = (ulong) range.max * 1000LU; //MHz

            debug("DEV%d FREQ DOM%d: %lu to %lu KHz (%u P_STATEs)", dv, d,
                   freqs_avail_list[dv][0], freqs_avail_list[dv][freqs_avail_count[dv]-1], freqs_avail_count[dv]-1);
            // We want just one domain
            break;
        }
    }
    return EAR_SUCCESS;
}

void mgt_gpu_oneapi_load(mgt_gpu_ops_t *ops)
{
    if (state_fail(oneapi_open_sysman(&ze))) {
        debug("oneapi_open_sysman failed: %s", state_msg);
        return;
    }
    if (state_fail(oneapi_get_handlers(&devs, &devs_count))) {
        debug("oneapi_get_devices failed: %s", state_msg);
        return;
    }
    // Check for FREQ and POWER DOMAINS.

    // Get static values
    get_available_frequencies();
    get_available_watts();
    debug("----------------------------------------");
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
        (*devs_in)[dv].serial = devs[dv].uuid;
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
    zes_freq_range_t range;
    int dv, d;

    for (dv = 0; dv < devs_count; ++dv) {
        if (khz != NULL) {
            khz[dv] = DUMMY_FREQ;
        }
        for (d = 0; d < devs[dv].sfreqs_count; ++d) {
            if (ze.sFrequencyGetRange(devs[dv].sfreqs[d], &range) != ZE_RESULT_SUCCESS) {
                continue;
            }
            debug("zesFrequencyGetRange D%d: %lf", dv, range.max);
            if (khz != NULL) {
                khz[dv] = ((ulong) range.max) * 1000LU;
            }
            break;
        }
    }
	return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_get_default(ctx_t *c, ulong *khz)
{
    int dv;
    for (dv = 0; dv < devs_count; ++dv) {
        if (freqs_default[dv] != NO_FREQ) {
            khz[dv] = freqs_default[dv];
        } else {
            khz[dv] = DUMMY_FREQ;
        }
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
    zes_freq_range_t range;
    int dv, d;
    // In OneAPI documentation: When setting limits to return to factory
    // settings, specify -1 for both the min and max limit.
    range.min = -1.0;
    range.max = -1.0;
    for (dv = 0; dv < devs_count; ++dv) {
        for (d = 0; d < devs[dv].sfreqs_count; ++d) {
            ze.sFrequencySetRange(devs[dv].sfreqs[d], &range);
        }
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_freq_limit_set(ctx_t *c, ulong *khz)
{
    zes_freq_range_t range;
    ze_result_t z;
    int dv, d;

    range.min = -1.0;
    for (dv = 0; dv < devs_count; ++dv) {
        for (d = 0; d < devs[dv].sfreqs_count; ++d) {
            range.max = ((double) khz[dv]) / 1000.0;
            if_fail(ze.sFrequencySetRange(devs[dv].sfreqs[d], &range), continue);
        }
    }
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
#if 0
    zes_power_sustained_limit_t ps;
    zes_power_burst_limit_t     pb;
    zes_power_peak_limit_t      pp;
    ze_result_t                 z;
    int                         dv;
    int                         d;

    for (dv = 0; dv < devs_count; ++dv) {
        watts[dv] = 0LU;
        if (devs[dv].spowers_count) {
            if_fail(ze.sPowerGetLimits(devs[dv].spowers[d], &ps, &pb, &pp),);
            watts[dv] = ((ulong) ps.power) / 1000LU; // mW
        }
    }
#endif
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
    // zesPowerSetLimits
    return EAR_SUCCESS;
}

state_t mgt_gpu_oneapi_power_cap_set(ctx_t *c, ulong *watts)
{
    zes_power_limit_ext_desc_t desc;
    uint                       one = 1;
    uint                       dv;

    for (dv = 0; dv < devs_count; ++dv) {
        // This device do not accepts power limits
        if (!power_descs[dv].enabled && power_descs[dv].enabledStateLocked) {
            continue;
        }
        memcpy(&desc, &power_descs[dv], sizeof(zes_power_limit_ext_desc_t));
        desc.limit = ((double) watts[dv]) * 1000.0;
        ze.sPowerSetLimitsExt(devs[dv].spowers[power_domains[dv]], &one, &power_descs[dv]);
    }
    return EAR_SUCCESS;
}
