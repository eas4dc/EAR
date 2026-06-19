/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/* clang-format off */
// #define SHOW_DEBUGS 1
#include <stdlib.h>
#include <string.h>
#include <common/output/debug.h>
#include <metrics/common/apis.h>
#include <metrics/common/offsets.h>
#include <metrics/gpu/archs/demo.h>

static ofops_t ops[3];
static uint    d;
static uint    devs_count;
static uint    scope;
static uint    granularity;

#define O1(l)    (offsetof(gpu_t, l))

static void load_everlasting()
{
    static int already_loaded = 0;
    size_t sz = sizeof(gpu_t);

    if (already_loaded) {
        return;
    }
    // Based in Nvidia A100
    ofops_add(&ops[0], ID_UINT64, 'r', sz, O1(freq_gpu),  200000,  100000,  0); // Idle
    ofops_add(&ops[0], ID_UINT64, 'r', sz, O1(freq_mem),  200000,  100000,  0); //
    ofops_add(&ops[0], ID_UINT64, 'l', sz, O1(util_gpu), 1410000,       0,  0); //
    ofops_add(&ops[0], ID_UINT64, 'l', sz, O1(util_mem), 1215000,       0,  0); //
    ofops_add(&ops[0], ID_DOUBLE, 'l', sz, O1(power_w ),   400.0,       0,  0); //
    ofops_add(&ops[1], ID_UINT64, 'r', sz, O1(freq_gpu), 1410000, 1000000,  0); // Gpu-intensive
    ofops_add(&ops[1], ID_UINT64, 'r', sz, O1(freq_mem), 1215000, 1000000,  0); //
    ofops_add(&ops[1], ID_UINT64, 'l', sz, O1(util_gpu), 1410000,       0,  0); //
    ofops_add(&ops[1], ID_UINT64, 'l', sz, O1(util_mem), 1215000,       0,  0); //
    ofops_add(&ops[1], ID_DOUBLE, 'l', sz, O1(power_w ),   400.0,       0,  0); //
    already_loaded = 1;
}

GPU_F_LOAD(demo)
{
    if (!API_IS(options, API_DEMO)) {
        return;
    }
    // If other API is loaded before, DEMO won't be
    if (api_already_loaded(ops)) {
        return;
    }
    // Initializations
    load_everlasting();
    devs_count  = 4;
    scope       = SCOPE_NODE;
    granularity = GRANULARITY_PERIPHERAL;
    apis_put(ops->unload   , gpu_demo_unload   );
    apis_put(ops->update   , gpu_demo_update   );
    apis_put(ops->get_info , gpu_demo_get_info );
    apis_put(ops->read     , gpu_demo_read     );
    apis_put(ops->data_diff, gpu_demo_data_diff);
}

GPU_F_UNLOAD(demo)
{
}

GPU_F_UPDATE(demo)
{
    if (option >= UPD_DEMO1_SET && option <= UPD_DEMO4_SET) {
        d = (option == UPD_DEMO4_SET)? 1: 0;
    }
    return_msg(EAR_ERROR, "option not available");
}

GPU_F_GET_INFO(demo)
{
    info->api         = API_DEMO;
    info->scope       = scope;
    info->granularity = granularity;
    info->devs_count  = devs_count;
}

GPU_F_READ(demo)
{
    timestamp_t now;
    int i;

    timestamp_getfast(&now);
    memset(d, 0, sizeof(gpu_t)*devs_count);
    for (i = 0; i < devs_count; ++i) {
        d[i].time = now;
    }
    return EAR_SUCCESS;
}

GPU_F_DATA_DIFF(demo)
{
    double time_f = timestamp_fdiff(&d2[0].time, &d1[0].time, TIME_SECS, TIME_MSECS);
    double util_gpu_f = 0.0;
    double util_mem_f = 0.0;
    int i;

    ofops_calc_array(&ops[d], (char *) dD, devs_count);
    for (i = 0; i < devs_count; ++i) {
        util_gpu_f = ((double) dD[i].freq_gpu) / ((double) dD[i].util_gpu);
        util_mem_f = ((double) dD[i].freq_mem) / ((double) dD[i].util_mem);
        dD[i].util_gpu = 100.0 * util_gpu_f;
        dD[i].util_mem = 100.0 * util_mem_f;
        dD[i].power_w  = dD[i].power_w * util_gpu_f;
        dD[i].energy_j = dD[i].power_w * time_f;
        dD[i].working  = 1;
        dD[i].correct  = 1;
        dD[i].samples  = 1;
    }
    #if SHOW_DEBUGS
    gpu_data_print(dD, fderr);
    #endif
}