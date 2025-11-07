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
// #define FAKE_GPUS   1

#include <common/output/debug.h>
#include <management/gpu/archs/dummy.h>
#include <stdlib.h>

#if FAKE_GPUS
#define FAKE_N 4
static ulong *clock_list[FAKE_N]; // MHz
static uint clock_lens[FAKE_N];
static ulong power_list_max[FAKE_N];
static ulong power_list_min[FAKE_N];
#else
static ulong clock_khz;
static ulong *clock_list[1];
static uint clock_lens[1];
#endif

void mgt_gpu_dummy_load(mgt_gpu_ops_t *ops)
{
    // Additional
    apis_add(ops->init, mgt_gpu_dummy_init);
    apis_add(ops->dispose, mgt_gpu_dummy_dispose);
    // Queries (put if empty)
    apis_put(ops->get_api, mgt_gpu_dummy_get_api);
    apis_put(ops->get_devices, mgt_gpu_dummy_get_devices);
    apis_put(ops->count_devices, mgt_gpu_dummy_count_devices);
    apis_put(ops->freq_limit_get_current, mgt_gpu_dummy_freq_limit_get_current);
    apis_put(ops->freq_limit_get_default, mgt_gpu_dummy_freq_limit_get_default);
    apis_put(ops->freq_limit_get_max, mgt_gpu_dummy_freq_limit_get_max);
    apis_put(ops->power_cap_get_current, mgt_gpu_dummy_power_cap_get_current);
    apis_put(ops->power_cap_get_default, mgt_gpu_dummy_power_cap_get_default);
    apis_put(ops->power_cap_get_rank, mgt_gpu_dummy_power_cap_get_rank);
    // Commands (put if empty)
    apis_put(ops->freq_limit_reset, mgt_gpu_dummy_freq_limit_reset);
    apis_put(ops->freq_limit_set, mgt_gpu_dummy_freq_limit_set);
    apis_put(ops->freq_list, mgt_gpu_dummy_freq_get_available);
    apis_put(ops->power_cap_reset, mgt_gpu_dummy_power_cap_reset);
    apis_put(ops->power_cap_set, mgt_gpu_dummy_power_cap_set);
    debug("Loaded DUMMY");
}

void mgt_gpu_dummy_get_api(uint *api)
{
#if FAKE_GPUS
    *api = API_FAKE;
#else
    *api = API_DUMMY;
#endif
}

state_t mgt_gpu_dummy_init(ctx_t *c)
{
#if FAKE_GPUS
    power_list_max[0] = 220;
    power_list_max[1] = 220;
    power_list_max[2] = 300;
    power_list_max[3] = 300;
    power_list_min[0] = 100;
    power_list_min[1] = 100;
    power_list_min[2] = 100;
    power_list_min[3] = 100;
    clock_lens[0]     = 10;
    clock_lens[1]     = 10;
    clock_lens[2]     = 12;
    clock_lens[3]     = 12;
    clock_list[0]     = calloc(clock_lens[0], sizeof(ulong));
    clock_list[1]     = calloc(clock_lens[1], sizeof(ulong));
    clock_list[2]     = calloc(clock_lens[2], sizeof(ulong));
    clock_list[3]     = calloc(clock_lens[3], sizeof(ulong));
    clock_list[0][0] = clock_list[1][0] = 1000000;
    clock_list[0][1] = clock_list[1][1] = 900000;
    clock_list[0][2] = clock_list[1][2] = 800000;
    clock_list[0][3] = clock_list[1][3] = 700000;
    clock_list[0][4] = clock_list[1][4] = 600000;
    clock_list[0][5] = clock_list[1][5] = 500000;
    clock_list[0][6] = clock_list[1][6] = 400000;
    clock_list[0][7] = clock_list[1][7] = 300000;
    clock_list[0][8] = clock_list[1][8] = 200000;
    clock_list[0][9] = clock_list[1][9] = 100000;
    clock_list[2][0] = clock_list[3][0] = 1200000;
    clock_list[2][1] = clock_list[3][1] = 1100000;
    clock_list[2][2] = clock_list[3][2] = 1000000;
    clock_list[2][3] = clock_list[3][3] = 900000;
    clock_list[2][4] = clock_list[3][4] = 800000;
    clock_list[2][5] = clock_list[3][5] = 700000;
    clock_list[2][6] = clock_list[3][6] = 600000;
    clock_list[2][7] = clock_list[3][7] = 500000;
    clock_list[2][8] = clock_list[3][8] = 400000;
    clock_list[2][9] = clock_list[3][9] = 300000;
    clock_list[2][10] = clock_list[3][10] = 200000;
    clock_list[2][11] = clock_list[3][11] = 100000;
#else
    clock_list[0] = &clock_khz;
    clock_lens[0] = 1;
    clock_khz     = 1000;
#endif
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_get_devices(ctx_t *c, gpu_devs_t **devs, uint *devs_count)
{
#if FAKE_GPUS
    *devs             = calloc(4, sizeof(gpu_devs_t));
    (*devs)[0].serial = (*devs)[0].index = 0;
    (*devs)[1].serial = (*devs)[1].index = 1;
    (*devs)[2].serial = (*devs)[2].index = 2;
    (*devs)[3].serial = (*devs)[3].index = 3;
#else
    *devs = calloc(1, sizeof(gpu_devs_t));
#endif
    return mgt_gpu_dummy_count_devices(c, devs_count);
}

state_t mgt_gpu_dummy_count_devices(ctx_t *c, uint *devs_count)
{
    if (devs_count != NULL) {
#if FAKE_GPUS
        *devs_count = FAKE_N;
#else
        *devs_count = 1;
#endif
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_freq_limit_get_current(ctx_t *c, ulong *khz)
{
    if (khz != NULL) {
#if FAKE_GPUS
        khz[0] = clock_list[0][0];
        khz[1] = clock_list[1][0];
        khz[2] = clock_list[2][0];
        khz[3] = clock_list[3][0];
#else
        khz[0] = 0;
#endif
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_freq_limit_get_default(ctx_t *c, ulong *khz)
{
    if (khz != NULL) {
#if FAKE_GPUS
        khz[0] = clock_list[0][0];
        khz[1] = clock_list[1][0];
        khz[2] = clock_list[2][0];
        khz[3] = clock_list[3][0];
#else
        khz[0] = 0;
#endif
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_freq_limit_get_max(ctx_t *c, ulong *khz)
{
    if (khz != NULL) {
#if FAKE_GPUS
        khz[0] = clock_list[0][0];
        khz[1] = clock_list[1][0];
        khz[2] = clock_list[2][0];
        khz[3] = clock_list[3][0];
#else
        khz[0] = 0;
#endif
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_freq_limit_reset(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_freq_limit_set(ctx_t *c, ulong *khz)
{
    debug("mgt_gpu_dummy_freq_limit_set");
#if FAKE_GPUS
    int i, j;

    debug("Received frequencies: ");

    for (i = 0; i < FAKE_N; ++i)
        for (j = 0; j < clock_lens[i]; ++j) {
            debug("D%d: %lu MHz", i, clock_list[i][j]);
        }
#endif

    if (khz != NULL) {
        khz[0] = 0;
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_freq_get_available(ctx_t *c, const ulong ***list_khz, const uint **list_len)
{
    if (list_khz != NULL) {
        *list_khz = (const ulong **) clock_list;
    }
    if (list_len != NULL) {
        *list_len = (const uint *) clock_lens;
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_power_cap_get_current(ctx_t *c, ulong *watts)
{
    if (watts != NULL) {
#if FAKE_GPUS
        watts[0] = power_list_max[0];
        watts[1] = power_list_max[1];
#else
        watts[0] = 0;
#endif
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_power_cap_get_default(ctx_t *c, ulong *watts)
{
    if (watts != NULL) {
#if FAKE_GPUS
        watts[0] = power_list_max[0];
        watts[1] = power_list_max[1];
        watts[2] = power_list_max[2];
        watts[3] = power_list_max[3];
#else
        watts[0] = 0;
#endif
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_power_cap_get_rank(ctx_t *c, ulong *watts_min, ulong *watts_max)
{
    if (watts_max != NULL) {
#if FAKE_GPUS
        watts_max[0] = power_list_max[0];
        watts_max[1] = power_list_max[1];
        watts_max[2] = power_list_max[2];
        watts_max[3] = power_list_max[3];
#else
        watts_max[0] = 0;
#endif
    }
    if (watts_min != NULL) {
#if FAKE_GPUS
        watts_min[0] = power_list_min[0];
        watts_min[1] = power_list_min[1];
        watts_min[2] = power_list_min[2];
        watts_min[3] = power_list_min[3];
#else
        watts_min[0] = 0;
#endif
    }
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_power_cap_reset(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t mgt_gpu_dummy_power_cap_set(ctx_t *c, ulong *watts)
{
    debug("mgt_gpu_dummy_power_cap_set");
    if (watts != NULL) {
        watts[0] = 0;
    }
    return EAR_SUCCESS;
}
