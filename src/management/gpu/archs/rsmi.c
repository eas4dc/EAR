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
#include <metrics/common/rsmi.h>
#include <management/gpu/archs/rsmi.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static rsmi_t          rsmi;
static uint			   devs_count;
static ulong          *clock_list_default; // KHz
static ulong		 **clock_list; // KHz
static uint			  *clock_lens;
static ulong          *pc_list_default; // W
static ulong          *pc_list_max; // W
static ulong          *pc_list_min; // W
static uint			   ok_init;

#define myErrorString(r) \
	((char *) rsmi.ErrorString(r))

int INDEX(uint length, uint index) {
    return length-index-1;
}

void mgt_gpu_rsmi_load(mgt_gpu_ops_t *ops)
{
    debug("Loading RSMI");
    if (state_fail(rsmi_open(&rsmi))) {
        debug("rsmi_open failed: %s", state_msg);
        return;
    }
    if (rsmi_fail(rsmi.devs_count(&devs_count))) {
        debug("rsmi.devs_count failed");
        return;
    }
    // Set always
    apis_set(ops->init                  , mgt_gpu_rsmi_init);
    apis_set(ops->dispose               , mgt_gpu_rsmi_dispose);
    // Queries
    apis_set(ops->get_api               , mgt_gpu_rsmi_get_api);
    apis_set(ops->get_devices,            mgt_gpu_rsmi_get_devices);
    apis_set(ops->count_devices         , mgt_gpu_rsmi_count_devices);
    apis_set(ops->freq_limit_get_current, mgt_gpu_rsmi_freq_limit_get_current);
    apis_set(ops->freq_limit_get_default, mgt_gpu_rsmi_freq_limit_get_default);
    apis_set(ops->freq_limit_get_max    , mgt_gpu_rsmi_freq_limit_get_max);
    apis_set(ops->freq_list             , mgt_gpu_rsmi_freq_list);
    apis_set(ops->power_cap_get_current , mgt_gpu_rsmi_power_cap_get_current);
    apis_set(ops->power_cap_get_default , mgt_gpu_rsmi_power_cap_get_default);
    apis_set(ops->power_cap_get_rank    , mgt_gpu_rsmi_power_cap_get_rank);
    // Checking if RSMI is capable to run commands
    if (!rsmi_is_privileged()) {
        goto done;
    }
    // Commands
    apis_set(ops->freq_limit_reset      , mgt_gpu_rsmi_freq_limit_reset);
    apis_set(ops->freq_limit_set        , mgt_gpu_rsmi_freq_limit_set);
    apis_set(ops->power_cap_reset       , mgt_gpu_rsmi_power_cap_reset);
    apis_set(ops->power_cap_set         , mgt_gpu_rsmi_power_cap_set);
done:
    debug("Loaded RSMI (%d devices)", devs_count);
}

void mgt_gpu_rsmi_get_api(uint *api)
{
    *api = API_RSMI;
}

static state_t static_dispose(ctx_t *c, state_t s, char *error)
{
    return EAR_SUCCESS;
}

#define static_alloc(v, l, s) \
	if ((v = calloc(l, s)) == NULL) { return static_dispose(NULL, EAR_ERROR, strerror(errno)); }

static state_t static_init_dev(int i)
{
    rsmi_freqs_t gpu_hz;
    rsmi_status_t s;
    int f;

    debug("Initializing device %d", i);
    // Getting frequency list per device 
    s = rsmi.get_clock(i, RSMI_CLK_TYPE_SYS, &gpu_hz);
    // Allocating device frequencies
    static_alloc(clock_list[i], gpu_hz.num_supported, sizeof(ulong));
    // Saving the default frequency (Hz to KHz)
    clock_list_default[i] = gpu_hz.frequency[gpu_hz.current] / 1000LU;
    // Saving the total number of device frequencies
    clock_lens[i] = gpu_hz.num_supported;
    // Copying (Hz to KHz)
    for (f = 0; f < clock_lens[i]; ++f) {
        clock_list[i][f] = gpu_hz.frequency[INDEX(clock_lens[i], f)] / 1000LU;
        debug("D%d F%d: %llu KHz", i, f, clock_list[i][f]);
    }
    // Powercap default (microWatts to Watts)
    rsmi.get_powercap_default(i, &pc_list_default[i]);
    pc_list_default[i] /= 1000000LU;
    // Powercap min/max (microWatts to Watts)
    rsmi.get_powercap_range(i, 0, &pc_list_max[i], &pc_list_min[i]); 
    pc_list_max[i] /= 1000000LU;
    pc_list_min[i] /= 1000000LU;
    //
    unused(s);
    #if SHOW_DEBUGS
    debug("D%d: %d frequencies", i, clock_lens[i]);
    for (f = 0; f < clock_lens[i]; ++f) {
        debug("D%d F%d: %lu KHz", i, f, clock_list[i][f]);
    }
    debug("D%d: powers between %lu - %lu, and defautl %lu", i, pc_list_min[i], pc_list_max[i], pc_list_default[i]);
    #endif

    return EAR_SUCCESS;
}

static state_t static_init()
{
    int d;

	static_alloc(pc_list_default,    devs_count, sizeof(ulong));
	static_alloc(pc_list_max,        devs_count, sizeof(ulong));
	static_alloc(pc_list_min,        devs_count, sizeof(ulong));
	static_alloc(clock_list_default, devs_count, sizeof(ulong));
	static_alloc(clock_list,         devs_count, sizeof(ulong *));
	static_alloc(clock_lens,         devs_count, sizeof(uint));

    for (d = 0; d < devs_count; ++d) {
        static_init_dev(d);
    }

    return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_init(ctx_t *c)
{
	state_t s = EAR_SUCCESS;
	while (pthread_mutex_trylock(&lock));
	if (!ok_init) {
		if (state_ok(s = static_init())) {
			ok_init = 1;
		}
	}
	pthread_mutex_unlock(&lock);
	return s;
}

state_t mgt_gpu_rsmi_dispose(ctx_t *c)
{
	return static_dispose(c, EAR_SUCCESS, NULL);
}

state_t mgt_gpu_rsmi_get_devices(ctx_t *c, gpu_devs_t **devs_in, uint *devs_count_in)
{
    char serial[32];
    rsmi_status_t r;
    int i;

    *devs_in = calloc(devs_count, sizeof(gpu_devs_t));
    //
    for (i = 0; i < devs_count; ++i) {
        if ((r = rsmi.get_serial(i, serial, 32)) != RSMI_STATUS_SUCCESS) {
            return_msg(EAR_ERROR, "RSMI error");
        }
        debug("D%d: %s", i, serial);
        (*devs_in)[i].serial = (ullong) atoll(serial);
        (*devs_in)[i].index  = i;
    }
    if (devs_count_in != NULL) {
        *devs_count_in = devs_count;
    }

    return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_count_devices(ctx_t *c, uint *devs_count_in)
{
	if (devs_count_in != NULL) {
		*devs_count_in = devs_count;
	}
	return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_freq_limit_get_current(ctx_t *c, ulong *khz)
{
    rsmi_freqs_t gpu_hz;
    rsmi_status_t s;
    int d;
    for (d = 0; d < devs_count; ++d) {
        // Getting frequency list per device 
        s = rsmi.get_clock(d, RSMI_CLK_TYPE_SYS, &gpu_hz);
        // Hz to KHz
        khz[d] = gpu_hz.frequency[INDEX(clock_lens[d], gpu_hz.current)] / 1000LU;
    }
    //
    unused(s);

	return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_freq_limit_get_default(ctx_t *c, ulong *khz)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
		khz[i] = clock_list_default[i];
	}
	return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_freq_limit_get_max(ctx_t *c, ulong *khz)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
		khz[i] = clock_list[i][clock_lens[i-1]];
	}
	return EAR_SUCCESS;
}

static state_t clocks_set(int i, ulong mhz)
{
    if (mhz == 0LU) {
        return EAR_SUCCESS;
    }
    rsmi.set_clock(i, RSMI_FREQ_IND_MAX, mhz, RSMI_CLK_TYPE_SYS);
	return EAR_SUCCESS;
}

static state_t clocks_reset(int i)
{
    return clocks_set(i, clock_list_default[i] * 1000LU);
}

state_t mgt_gpu_rsmi_freq_limit_reset(ctx_t *c)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
        clocks_reset(i);
	}
	return EAR_SUCCESS;
}

/* If freq == 0 for a given GPU, it's not changed */
state_t mgt_gpu_rsmi_freq_limit_set(ctx_t *c, ulong *khz)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
        clocks_set(i, khz[i] * 1000);
	}
	return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_freq_list(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
	if (list_khz != NULL) {
		*list_khz = (const ulong **) clock_list;
	}
	if (list_len != NULL) {
		*list_len = (const uint *) clock_lens;
	}
	return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_power_cap_get_current(ctx_t *c, ulong *watts)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
        rsmi.get_powercap(i, 0, &watts[i]);
        watts[i] /= 1000000LU;
	}
	return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_power_cap_get_default(ctx_t *c, ulong *watts)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
		watts[i] = pc_list_default[i];
	}
	return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
		watts_min[i] = pc_list_min[i];
		watts_max[i] = pc_list_max[i];
	}
	return EAR_SUCCESS;
}

static state_t powercap_set(int i, ulong uw)
{
    if (uw == 0LU) {
        return EAR_SUCCESS;
    }
    rsmi.set_powercap(i, 0, uw);
	return EAR_SUCCESS;
}

static state_t powercap_reset(int i)
{
    // Watts to microWatts
	return powercap_set(i, pc_list_default[i] * 1000000LU);
}

state_t mgt_gpu_rsmi_power_cap_reset(ctx_t *c)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
        powercap_reset(i);
	}
	return EAR_SUCCESS;
}

state_t mgt_gpu_rsmi_power_cap_set(ctx_t *c, ulong *watts)
{
	int i;
	for (i = 0; i < devs_count; ++i) {
        // Watts to microWatts
        powercap_set(i, watts[i] * 1000000LU);
	}
	return EAR_SUCCESS;
}
