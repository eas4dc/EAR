/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/config/config_env.h>
#include <common/output/debug.h>
#include <common/system/symplug.h>
#include <management/gpu/archs/nvml.h>
#include <metrics/common/nvml.h>
#include <stdlib.h>
#include <unistd.h>

static nvml_t nvml;
static uint devs_count;
static nvmlDevice_t *devs;
static ulong *clock_max_default; // KHz
static ulong *clock_max;         // KHz
static ulong *clock_max_mem;     // MHz
static ulong **clock_list;       // KHz
static uint *clock_lens;
static ulong *power_max_default; // W
static ulong *power_max;         // W
static ulong *power_min;         // W

#define myErrorString(r) ((char *) nvml.ErrorString(r))

static state_t static_dispose(state_t s, char *error)
{
    int d;

#define static_free(v)                                                                                                 \
    if (v != NULL) {                                                                                                   \
        free(v);                                                                                                       \
        v = NULL;                                                                                                      \
    }

    for (d = 0; d < devs_count; ++d) {
        static_free(clock_list[d]);
    }
    static_free(clock_max_default);
    static_free(clock_max);
    static_free(clock_max_mem);
    static_free(power_max_default);
    static_free(power_max);
    static_free(power_min);
    static_free(clock_list);
    static_free(clock_lens);
    return_msg(EAR_ERROR, error);
}

static state_t static_init()
{
    nvmlReturn_t r;
    uint aux_mem[1000];
    uint aux_gpu[1000];
    uint c1, c2, aux;
    int d, m, g;

// Allocation
#define static_alloc(v, l, s)                                                                                          \
    if ((v = calloc(l, s)) == NULL) {                                                                                  \
        return static_dispose(EAR_ERROR, strerror(errno));                                                             \
    }

    static_alloc(clock_max_default, devs_count, sizeof(ulong));
    static_alloc(clock_max, devs_count, sizeof(ulong));
    static_alloc(clock_max_mem, devs_count, sizeof(ulong));
    static_alloc(power_max_default, devs_count, sizeof(ulong));
    static_alloc(power_max, devs_count, sizeof(ulong));
    static_alloc(power_min, devs_count, sizeof(ulong));
    static_alloc(clock_list, devs_count, sizeof(ulong *));
    static_alloc(clock_lens, devs_count, sizeof(uint));

    // Power fill
    debug("power list:");
    for (d = 0; d < devs_count; ++d) {
        if ((r = nvml.GetPowerDefaultLimit(devs[d], &aux)) != NVML_SUCCESS) {
            debug("nvmlDeviceGetPowerManagementDefaultLimit returned %d (%s)", r, nvml.ErrorString(r));
            return static_dispose(EAR_ERROR, myErrorString(r));
        }
        if ((r = nvml.GetPowerLimitConstraints(devs[d], &c1, &c2)) != NVML_SUCCESS) {
            debug("nvmlDeviceGetPowerManagementLimitConstraints returned %d (%s)", r, nvml.ErrorString(r));
            return static_dispose(EAR_ERROR, myErrorString(r));
        }
        power_max_default[d] = ((ulong) aux) / 1000LU;
        power_max[d]         = ((ulong) c2) / 1000LU;
        power_min[d]         = ((ulong) c1) / 1000LU;
        debug("D%d default: %lu W", d, power_max_default[d]);
        debug("D%d new max: %lu W", d, power_max[d]);
        debug("D%d new min: %lu W", d, power_min[d]);
    }
    // Clocks fill
    debug("clock list:");
    for (d = 0; d < devs_count; ++d) {
        clock_max_default[d] = 0LU;
        clock_max[d]         = 0LU;

        // Retrieving default application clocks
        if ((r = nvml.GetDefaultAppsClock(devs[d], NVML_CLOCK_GRAPHICS, &aux)) != NVML_SUCCESS) {
            debug("nvmlDeviceGetDefaultApplicationsClock(dev: %d) returned %d (%s)", d, r, nvml.ErrorString(r));
            return static_dispose(EAR_ERROR, myErrorString(r));
        }

        clock_max_default[d] = ((ulong) aux) * 1000LU;
        debug("D%d default: %lu KHz", d, clock_max_default[d]);

        /*
         * Retrieving a list of clock P_STATEs and saving its maximum.
         */
        c1 = 1000;
        if ((r = nvml.GetMemoryClocks(devs[d], &c1, aux_mem)) != NVML_SUCCESS) {
            debug("nvmlDeviceGetSupportedMemoryClocks(dev: %d) returned %d (%s)", d, r, nvml.ErrorString(r));
            return static_dispose(EAR_ERROR, myErrorString(r));
        }
        for (m = 0; m < c1; ++m) {
            if (aux_mem[m] > clock_max_mem[d]) {
                clock_max_mem[d] = (ulong) aux_mem[m];
            }
            debug("D%d MEM %d: %d (%u)", d, m, aux_mem[m], c1);

            c2 = 1000;
            if ((r = nvml.GetGraphicsClocks(devs[d], aux_mem[m], &c2, aux_gpu)) != NVML_SUCCESS) {
                debug("\t\tnvmlDeviceGetSupportedGraphicsClocks(dev: %d) returned %d (%s)", d, r, nvml.ErrorString(r));
                return static_dispose(EAR_ERROR, myErrorString(r));
            }

            for (g = 0; g < c2; ++g) {
                if (aux_gpu[g] > clock_max[d]) {
                    debug("D%d new max: %d MHz", d, aux_gpu[g]);
                    clock_max[d] = (ulong) aux_gpu[g];
                } else {
                    debug("D%d: %d MHz", d, aux_gpu[g]);
                }
            }
        }
        clock_max[d] *= 1000LU;
    }
    // P_STATEs fill
    for (d = 0; d < devs_count; ++d) {
        clock_lens[d] = 1000;
        if ((r = nvml.GetGraphicsClocks(devs[d], clock_max_mem[d], &clock_lens[d], aux_gpu)) != NVML_SUCCESS) {
            clock_lens[d] = 0;
            return static_dispose(EAR_ERROR, myErrorString(r));
        }
        clock_list[d] = calloc(clock_lens[d], sizeof(ulong *));
        for (m = 0; m < clock_lens[d]; ++m) {
            clock_list[d][m] = ((ulong) aux_gpu[m]) * 1000LU;
#if 0
            debug("D%u,M%u: %lu", d, m, clock_list[d][m]);
#endif
        }
    }

#if 0
    // Full clocks reset (application + locked)
	for (d = 0; d < devs_count; ++d) {
		if ((r = nvml.ResetAppsClocks(devs[d])) != NVML_SUCCESS) {
			debug("nvmlDeviceResetApplicationsClocks(dev: %d) returned %d (%s)",
				  d, r, nvml.ErrorString(r));
			return static_dispose(EAR_ERROR, myErrorString(r));
		}
		if ((r = nvml.ResetLockedClocks(devs[d])) != NVML_SUCCESS) {
			debug("nvmlDeviceResetGpuLockedClocks(dev: %d) returned %d (%s)",
				  d, r, nvml.ErrorString(r));
			//return_msg(EAR_ERROR, myErrorString(r));
		}
	}
	// Power limit reset
	for (d = 0; d < devs_count; ++d) {
		aux = ((uint) power_max_default[d]) * 1000U;
		if ((r = nvml.SetPowerLimit(devs[d], aux)) != NVML_SUCCESS) {
			debug("nvmlDeviceSetPowerManagementLimit(dev: %d) returned %d (%s)",
				  d, r, nvml.ErrorString(r));
			return static_dispose(EAR_ERROR, myErrorString(r));
		}
	}
#endif
    return EAR_SUCCESS;
}

void mgt_gpu_nvml_load(mgt_gpu_ops_t *ops)
{
    if (state_fail(nvml_open(&nvml))) {
        debug("nvml_open failed: %s", state_msg);
        return;
    }
    if (state_fail(nvml_get_devices(&devs, &devs_count))) {
        debug("nvml_get_devices failed: %s", state_msg);
        return;
    }
    if (state_fail(static_init())) {
        return;
    }
    // Set always
    apis_set(ops->init, mgt_gpu_nvml_init);
    apis_set(ops->dispose, mgt_gpu_nvml_dispose);
    // Queries
    apis_set(ops->get_api, mgt_gpu_nvml_get_api);
    apis_set(ops->get_devices, mgt_gpu_nvml_get_devices);
    apis_set(ops->count_devices, mgt_gpu_nvml_count_devices);
    apis_set(ops->freq_limit_get_current, mgt_gpu_nvml_freq_limit_get_current);
    apis_set(ops->freq_limit_get_default, mgt_gpu_nvml_freq_limit_get_default);
    apis_set(ops->freq_limit_get_max, mgt_gpu_nvml_freq_limit_get_max);
    apis_set(ops->freq_list, mgt_gpu_nvml_freq_list);
    apis_set(ops->power_cap_get_current, mgt_gpu_nvml_power_cap_get_current);
    apis_set(ops->power_cap_get_default, mgt_gpu_nvml_power_cap_get_default);
    apis_set(ops->power_cap_get_rank, mgt_gpu_nvml_power_cap_get_rank);
    // Checking if NVML is capable to run commands
    if (!nvml_is_privileged()) {
        goto done;
    }
    // Commands
    apis_set(ops->freq_limit_reset, mgt_gpu_nvml_freq_limit_reset);
    apis_set(ops->freq_limit_set, mgt_gpu_nvml_freq_limit_set);
    apis_set(ops->power_cap_reset, mgt_gpu_nvml_power_cap_reset);
    apis_set(ops->power_cap_set, mgt_gpu_nvml_power_cap_set);
done:
    debug("Loaded NVML");
}

void mgt_gpu_nvml_get_api(uint *api)
{
    *api = API_NVML;
}

state_t mgt_gpu_nvml_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_nvml_dispose(ctx_t *c)
{
    return static_dispose(EAR_SUCCESS, NULL);
}

state_t mgt_gpu_nvml_get_devices(ctx_t *c, gpu_devs_t **devs_in, uint *devs_count_in)
{
    char serial[32];
    nvmlReturn_t r;
    int i;

    *devs_in = calloc(devs_count, sizeof(gpu_devs_t));
    //
    for (i = 0; i < devs_count; ++i) {
        if ((r = nvml.GetSerial(devs[i], serial, 32)) != NVML_SUCCESS) {
            return_msg(EAR_ERROR, myErrorString(r));
        }
        (*devs_in)[i].serial = (ullong) atoll(serial);
        (*devs_in)[i].index  = i;
    }
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }

    return EAR_SUCCESS;
}

state_t mgt_gpu_nvml_count_devices(ctx_t *c, uint *devs_count_in)
{
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_nvml_freq_limit_get_current(ctx_t *c, ulong *khz)
{
    uint mhz;
    int i;
    for (i = 0; i < devs_count; ++i) {
        nvml.GetClock(devs[i], NVML_CLOCK_GRAPHICS, NVML_CLOCK_ID_APP_CLOCK_TARGET, &mhz);
        debug("NVML_CLOCK_ID_APP_CLOCK_TARGET %u KHz", mhz * 1000U);
        khz[i] = ((ulong) mhz) * 1000LU;
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_nvml_freq_limit_get_default(ctx_t *c, ulong *khz)
{
    int i;
    for (i = 0; i < devs_count; ++i) {
        khz[i] = clock_max_default[i];
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_nvml_freq_limit_get_max(ctx_t *c, ulong *khz)
{
    int i;
    for (i = 0; i < devs_count; ++i) {
        khz[i] = clock_max[i];
        debug("D%d max: %lu KHz", i, khz[i]);
    }
    return EAR_SUCCESS;
}

static state_t clocks_reset(int i)
{
    nvmlReturn_t r;
    debug("resetting clocks of device %d", i);

#if 0
	if ((r = nvml.ResetLockedClocks(devs[i])) != NVML_SUCCESS)
	{
		debug("nvmlDeviceResetGpuLockedClocks(dev: %d) returned %d (%s)",
			  i, r, nvml.ErrorString(r));
#endif
    if ((r = nvml.ResetAppsClocks(devs[i])) != NVML_SUCCESS) {
        return_msg(EAR_ERROR, myErrorString(r));
    }
#if 0
	}
#endif
    return EAR_SUCCESS;
}

state_t mgt_gpu_nvml_freq_limit_reset(ctx_t *c)
{
    state_t s, e;
    int i;
    for (i = 0, e = EAR_SUCCESS; i < devs_count; ++i) {
        if (state_fail(s = clocks_reset(i))) {
            e = s;
        }
    }
    return e;
}

static state_t clocks_set(int i, uint mhz)
{
    nvmlReturn_t r;
    uint parsed_mhz;
    ulong aux;

    if (mhz == 0U) {
        return EAR_SUCCESS;
    }
    mgt_gpu_freq_get_valid(NULL, i, ((ulong) mhz) * 1000LU, &aux);
    parsed_mhz = ((uint) aux) / 1000U;
    debug("D%d setting clock %u KHz (parsed => %u)", i, mhz * 1000U, parsed_mhz * 1000U);
#if 0
	if ((r = nvml.SetLockedClocks(devs[i], 0, parsed_mhz)) != NVML_SUCCESS) {
		debug("nvmlDeviceSetGpuLockedClocks(dev: %d) returned %d (%s)",
			i, r, nvml.ErrorString(r));
#endif
    if ((r = nvml.SetAppsClocks(devs[i], clock_max_mem[i], parsed_mhz)) != NVML_SUCCESS) {
        debug("nvmlDeviceSetApplicationsClocks(dev: %d) returned %d (%s)", i, r, nvml.ErrorString(r));
        // Unlike POWER functions, which have a function ask about the current
        // POWER LIMIT value, CLOCK functions does not have that possibilty. So
        // If the CLOCK SET function fails, it will try to set the default clock
        // frequency, which we already know its value.
        clocks_reset(i);
        //
        return_msg(EAR_ERROR, myErrorString(r));
    }
#if 0
	}
#endif
    return EAR_SUCCESS;
}

/* If freq == 0 for a given GPU, it's not changed */
state_t mgt_gpu_nvml_freq_limit_set(ctx_t *c, ulong *khz)
{
    state_t s, e;
    int i;
    debug("nvml_clock_limit_set devices %d", devs_count);
    for (i = 0, e = EAR_SUCCESS; i < devs_count; ++i) {
        if (state_fail(s = clocks_set(i, (uint) (khz[i] / 1000LU)))) {
            e = s;
        }
    }
    return e;
}

state_t mgt_gpu_nvml_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
    debug("nvml_freq_list");
    if (list_khz != NULL) {
        *list_khz = (const ulong **) clock_list;
    }
    if (list_len != NULL) {
        *list_len = (const uint *) clock_lens;
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_nvml_power_cap_get_current(ctx_t *c, ulong *watts)
{
    nvmlReturn_t r;
    state_t e;
    uint aux;
    int i;

    for (i = 0, e = EAR_SUCCESS; i < devs_count; ++i) {
        if ((r = nvml.GetPowerLimit(devs[i], &aux)) != NVML_SUCCESS) {
            debug("nvmlDeviceGetPowerManagementLimit(dev: %d) returned %d (%s)", i, r, nvml.ErrorString(r));
            // Setting error and unknown value parameter
            watts[i]  = 0;
            state_msg = (char *) nvml.ErrorString(r);
            e         = EAR_ERROR;
        }
        watts[i] = aux / 1000u;
    }
    return e;
}

state_t mgt_gpu_nvml_power_cap_get_default(ctx_t *c, ulong *watts)
{
    int i;
    for (i = 0; i < devs_count; ++i) {
        watts[i] = power_max_default[i];
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_nvml_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
    int i;
    for (i = 0; i < devs_count && watts_max != NULL; ++i) {
        watts_max[i] = power_max[i];
    }
    for (i = 0; i < devs_count && watts_min != NULL; ++i) {
        watts_min[i] = power_min[i];
    }
    return EAR_SUCCESS;
}

static state_t powers_set(int i, uint mw)
{
    nvmlReturn_t r;

    if (mw == 0U) {
        return EAR_SUCCESS;
    }
    debug("D%d setting power to %u mW", i, mw);
    if ((r = nvml.SetPowerLimit(devs[i], mw)) != NVML_SUCCESS) {
        debug("nvmlDeviceSetPowerManagementLimit(dev: %d) returned %d (%s)", i, r, nvml.ErrorString(r));
        return_msg(EAR_ERROR, myErrorString(r));
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_nvml_power_cap_reset(ctx_t *c)
{
    return mgt_gpu_nvml_power_cap_set(c, power_max_default);
}

state_t mgt_gpu_nvml_power_cap_set(ctx_t *c, ulong *watts)
{
    state_t s, e;
    int i;

    for (i = 0, e = EAR_SUCCESS; i < devs_count; ++i) {
        if (state_fail(s = powers_set(i, (uint) (watts[i] * 1000LU)))) {
            e = s;
        }
    }
    return e;
}
