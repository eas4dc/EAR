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
#include <metrics/gpu/archs/dummy.h>
#include <stdlib.h>

#if FAKE_GPUS
#define FAKE_N 4
#endif

void gpu_dummy_load(gpu_ops_t *ops)
{
    apis_put(ops->get_info, gpu_dummy_get_info);
    apis_put(ops->get_devices, gpu_dummy_get_devices);
    apis_put(ops->init, gpu_dummy_init);
    apis_put(ops->dispose, gpu_dummy_dispose);
    apis_put(ops->read, gpu_dummy_read);
    apis_put(ops->read_raw, gpu_dummy_read_raw);
}

void gpu_dummy_get_info(apinfo_t *info)
{
#if FAKE_GPUS
    info->api        = API_FAKE;
    info->devs_count = FAKE_N;
#else
    info->api        = API_DUMMY;
    info->devs_count = 1;
#endif
}

void gpu_dummy_get_devices(gpu_devs_t **devs, uint *devs_count)
{
    if (devs != NULL) {
#if FAKE_GPUS
        *devs             = calloc(4, sizeof(gpu_devs_t));
        (*devs)[0].serial = (*devs)[0].index = 0;
        (*devs)[1].serial = (*devs)[1].index = 1;
        (*devs)[2].serial = (*devs)[2].index = 2;
        (*devs)[3].serial = (*devs)[3].index = 3;
#else
        *devs = calloc(1, sizeof(gpu_devs_t));
#endif
    }
    if (devs_count != NULL) {
#if FAKE_GPUS
        *devs_count = FAKE_N;
#else
        *devs_count = 1;
#endif
    }
}

state_t gpu_dummy_init(ctx_t *c)
{
    return EAR_SUCCESS;
}

state_t gpu_dummy_dispose(ctx_t *c)
{
    return EAR_SUCCESS;
}

#if FAKE_GPUS
static state_t fake_read(gpu_t *data)
{
    timestamp_t time;
    int r;

    timestamp_getfast(&time);

    for (r = 0; r < FAKE_N; ++r) {
        data[r].time = time;
        data[r].samples += 1;
        data[r].freq_gpu += r + 1;
        data[r].freq_mem += r + 1;
        data[r].util_gpu += r + 20;
        data[r].util_mem += r + 20;
        data[r].temp_gpu += r + 70;
        data[r].temp_mem += r + 70;
        data[r].energy_j = 0;
        data[r].power_w += r + 100;
        data[r].working = 1;
        data[r].correct = 1;
    }
    return EAR_SUCCESS;
}
#endif

state_t gpu_dummy_read(ctx_t *c, gpu_t *data)
{
    debug("gpu_dummy_read");
#if FAKE_GPUS
    return fake_read(data);
#else
    gpu_data_null(data);
    return EAR_SUCCESS;
#endif
}

state_t gpu_dummy_read_raw(ctx_t *c, gpu_t *data)
{
    debug("gpu_dummy_read_raw");
    return gpu_dummy_read(c, data);
}